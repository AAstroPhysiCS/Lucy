#include "Mesh.h"

namespace Lucy {

	constexpr static uint32_t ASSIMP_FLAGS = aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals |
		aiProcess_FixInfacingNormals |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ImproveCacheLocality |
		aiProcess_LimitBoneWeights |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_ValidateDataStructure |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		//aiProcess_PreTransformVertices | (animations won't work, if you enable this)
		aiProcess_SplitLargeMeshes |
		aiProcess_OptimizeMeshes;

	Mesh::Mesh(const std::string& path)
		: m_Path(path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, ASSIMP_FLAGS);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
		{
			LUCY_CRITICAL("Mesh could not be imported!");
			return;
		}

		uint32_t totalSize = 0;
		m_IndexBuffer = IndexBuffer::Create();
		LoadData(scene, totalSize);
		LoadMaterials(scene);
		TraverseHierarchy(scene->mRootNode, nullptr);

		m_VertexBuffer = VertexBuffer::Create(totalSize * 15);
		float* dataPtr = m_VertexBuffer->GetData();
		uint32_t vertPtr = 0;
		
		for (Submesh& submesh : m_Submeshes) {
			auto& vertices = submesh.Vertices;
			auto& normals = submesh.Normals;
			auto& textureCoords = submesh.TextureCoords;
			auto& tangents = submesh.Tangents;
			auto& biTangents = submesh.BiTangents;

			for (uint32_t i = 0; i < submesh.VertexCount; i++) {
				dataPtr[vertPtr++] = vertices[i].x;
				dataPtr[vertPtr++] = vertices[i].y;
				dataPtr[vertPtr++] = vertices[i].z;
				dataPtr[vertPtr++] = 0.0f; //SubmeshID

				dataPtr[vertPtr++] = textureCoords[i].x;
				dataPtr[vertPtr++] = textureCoords[i].y;

				dataPtr[vertPtr++] = normals[i].x;
				dataPtr[vertPtr++] = normals[i].y;
				dataPtr[vertPtr++] = normals[i].z;

				dataPtr[vertPtr++] = tangents[i].x;
				dataPtr[vertPtr++] = tangents[i].y;
				dataPtr[vertPtr++] = tangents[i].z;

				dataPtr[vertPtr++] = biTangents[i].x;
				dataPtr[vertPtr++] = biTangents[i].y;
				dataPtr[vertPtr++] = biTangents[i].z;
			}
		}
	}

	RefLucy<Mesh> Mesh::Create(const std::string& path)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
		case RenderContextType::OpenGL:
			return CreateRef<Mesh>(path);
			break;
		case RenderContextType::Vulkan:
			LUCY_CRITICAL("Vulkan not supported");
			LUCY_ASSERT(false);
			break;
		}
	}

	void Mesh::LoadData(const aiScene* scene, uint32_t& totalSize)
	{
		aiMesh** meshes = scene->mMeshes;
		uint32_t meshCount = scene->mNumMeshes;

		uint32_t baseVertexCount = 0;
		uint32_t baseIndexCount = 0;

		for (uint32_t i = 0; i < meshCount; i++) {
			aiMesh* mesh = meshes[i];
			Submesh submesh;
			submesh.BaseVertexCount = baseVertexCount;
			submesh.BaseIndexCount = baseIndexCount;
			submesh.VertexCount = mesh->mNumVertices;
			submesh.IndexCount = mesh->mNumFaces * 3;

			baseVertexCount += submesh.VertexCount;
			baseIndexCount += submesh.IndexCount;

			uint32_t sizeVertices = submesh.VertexCount;
			totalSize += sizeVertices;

			if (mesh->HasPositions()) {
				aiVector3D* vertices = mesh->mVertices;
				submesh.Vertices.resize(sizeVertices);
				memcpy(submesh.Vertices.data(), vertices, sizeVertices * sizeof(aiVector3D));
			}

			if (mesh->HasNormals()) {
				aiVector3D* normals = mesh->mNormals;
				submesh.Normals.resize(sizeVertices);
				memcpy(submesh.Normals.data(), normals, sizeVertices * sizeof(aiVector3D));
			}

			if (mesh->HasTextureCoords(0)) {
				aiVector3D* textureCoords = mesh->mTextureCoords[0];
				submesh.TextureCoords.resize(sizeVertices);
				memcpy(submesh.TextureCoords.data(), textureCoords, sizeVertices * sizeof(aiVector2D));
			}

			if (mesh->HasTangentsAndBitangents()) {
				aiVector3D* tangents = mesh->mTangents;
				submesh.Tangents.resize(sizeVertices);
				memcpy(submesh.Tangents.data(), tangents, sizeVertices * sizeof(aiVector3D));

				aiVector3D* biTangents = mesh->mBitangents;
				submesh.BiTangents.resize(sizeVertices);
				memcpy(submesh.BiTangents.data(), biTangents, sizeVertices * sizeof(aiVector3D));
			}

			submesh.MaterialIndex = mesh->mMaterialIndex;

			std::vector<uint32_t> indices;
			if (mesh->HasFaces()) {
				for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
					aiFace aiFace = mesh->mFaces[i];
					for (uint32_t j = 0; j < aiFace.mNumIndices; j++) {
						indices.push_back(aiFace.mIndices[j]);
					}
				}
			}
			m_IndexBuffer->SetData(indices.data(), indices.size(), 0);

			//TODO: Animation

			m_Submeshes.push_back(submesh);
		}
	}

	void Mesh::LoadMaterials(const aiScene* scene)
	{
		aiMaterial** materials = scene->mMaterials;
		size_t materialSize = scene->mNumMaterials;

		for (uint32_t i = 0; i < materialSize; i++) {
			aiMaterial* material = materials[i];
			m_Materials.push_back(Material(material, m_Path));
		}
	}

	void Mesh::TraverseHierarchy(const aiNode* node, const aiNode* rootNode)
	{
		aiMatrix4x4 transform = node->mTransformation;
		if (rootNode) {
			transform *= rootNode->mTransformation;
		}

		for (uint32_t i = 0; i < node->mNumMeshes; i++) {
			Submesh submesh = m_Submeshes[node->mMeshes[i]];
			submesh.Transform = *(glm::mat4*)&transform;
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++) {
			aiNode* childrenNode = node->mChildren[i];
			TraverseHierarchy(childrenNode, node);
		}
	}
	
}