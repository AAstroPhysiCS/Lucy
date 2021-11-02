#include "Mesh.h"

#include "../Core/Timer.h"
#include "Renderer.h"

#include "RenderPass.h"
#include "RenderCommand.h"
#include "Context/OpenGLPipeline.h"

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
		ScopedTimer scopedTimer;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, ASSIMP_FLAGS);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
		{
			LUCY_CRITICAL("Mesh could not be imported!");
			return;
		}

		uint32_t totalVertexSize = 0, totalIndexSize = 0;
		LoadData(scene, totalVertexSize, totalIndexSize);
		LoadMaterials(scene);
		TraverseHierarchy(scene->mRootNode, nullptr);
		
		m_IndexBuffer = IndexBuffer::Create(totalIndexSize);
		for (Submesh& submesh : m_Submeshes) {
			m_IndexBuffer->AddData(submesh.Faces);
		}
		m_VertexBuffer = VertexBuffer::Create(totalVertexSize * 15);
		float* dataPtr = m_VertexBuffer->GetData();

		uint32_t vertPtr = 0;
		for (Submesh& submesh : m_Submeshes) {
			auto& vertices = submesh.Vertices;
			auto& textureCoords = submesh.TextureCoords;
			auto& normals = submesh.Normals;
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

	Mesh::~Mesh()
	{
		m_VertexBuffer->Destroy();
		m_IndexBuffer->Destroy();

		if (Renderer::GetCurrentRenderAPI() == RenderAPI::OpenGL) {
			glDeleteVertexArrays(1, &m_Vao);
		}
	}

	RefLucy<Mesh> Mesh::Create(const std::string& path)
	{
		switch (Renderer::GetCurrentRenderAPI()) {
			case RenderAPI::OpenGL:
				return CreateRef<Mesh>(path);
				break;
			case RenderAPI::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}

	void Mesh::Bind()
	{
		if (!m_Loaded)
			LoadBuffers();

		if (Renderer::GetCurrentRenderAPI() == RenderAPI::OpenGL) {
			glBindVertexArray(m_Vao);
			m_IndexBuffer->Bind();
		}
	}

	void Mesh::Unbind()
	{
		if (Renderer::GetCurrentRenderAPI() == RenderAPI::OpenGL) {
			glBindVertexArray(0);
			m_IndexBuffer->Unbind();
		}
	}

	void Mesh::LoadBuffers()
	{
		if (Renderer::GetCurrentRenderAPI() == RenderAPI::OpenGL) {
			glCreateVertexArrays(1, &m_Vao);
			glBindVertexArray(m_Vao);

			RefLucy<OpenGLPipeline>& pipeline = As(RenderCommand::s_ActiveRenderPass->GetPipeline(), OpenGLPipeline);
			pipeline->UploadVertexLayout(As(m_VertexBuffer, OpenGLVertexBuffer));

			m_VertexBuffer->Load();
			m_IndexBuffer->Load();
			
			glBindVertexArray(0);
		}

		m_Loaded = true;
	}

	void Mesh::LoadData(const aiScene* scene, uint32_t& totalVertexSize, uint32_t& totalIndexSize)
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

			totalVertexSize += submesh.VertexCount;
			totalIndexSize += submesh.IndexCount;

			uint32_t sizeVertices = submesh.VertexCount;

			if (mesh->HasPositions()) {
				aiVector3D* vertices = mesh->mVertices;
				submesh.Vertices.resize(sizeVertices);
				memcpy(submesh.Vertices.data(), vertices, sizeVertices);
			}

			if (mesh->HasNormals()) {
				aiVector3D* normals = mesh->mNormals;
				submesh.Normals.resize(sizeVertices);
				memcpy(submesh.Normals.data(), normals, sizeVertices);
			}

			if (mesh->HasTextureCoords(0)) {
				aiVector3D* textureCoords = mesh->mTextureCoords[0];
				submesh.TextureCoords.resize(sizeVertices);
				for (uint32_t j = 0; j < sizeVertices / 3; j++) {
					submesh.TextureCoords.push_back({ textureCoords[j].x, textureCoords[j].y });
				}
			}

			if (mesh->HasTangentsAndBitangents()) {
				aiVector3D* tangents = mesh->mTangents;
				submesh.Tangents.resize(sizeVertices);
				memcpy(submesh.Tangents.data(), tangents, sizeVertices);

				aiVector3D* biTangents = mesh->mBitangents;
				submesh.BiTangents.resize(sizeVertices);
				memcpy(submesh.BiTangents.data(), biTangents, sizeVertices);
			}

			submesh.MaterialIndex = mesh->mMaterialIndex;

			if (mesh->HasFaces()) {
				submesh.Faces.reserve(submesh.IndexCount);
				for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
					aiFace aiFace = mesh->mFaces[i];
					for (uint32_t j = 0; j < aiFace.mNumIndices; j++) {
						submesh.Faces.emplace_back(aiFace.mIndices[j]);
					}
				}
			}

			//TODO: Animation

			m_Submeshes.push_back(submesh);
		}
	}

	void Mesh::LoadMaterials(const aiScene* scene)
	{
		aiMaterial** materials = scene->mMaterials;
		size_t materialSize = scene->mNumMaterials;

		m_Materials.reserve(materialSize);

		for (uint32_t i = 0; i < materialSize; i++) {
			aiMaterial* material = materials[i];
			m_Materials.emplace_back(Renderer::GetShaderLibrary().GetShader("LucyPBR"), material, m_Path);
		}
	}

	void Mesh::TraverseHierarchy(const aiNode* node, const aiNode* rootNode)
	{
		aiMatrix4x4 transform = node->mTransformation;
		if (rootNode) {
			transform *= rootNode->mTransformation;
		}

		for (uint32_t i = 0; i < node->mNumMeshes; i++) {
			Submesh& submesh = m_Submeshes[node->mMeshes[i]];
			submesh.Transform = glm::mat4(1.0f);
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++) {
			aiNode* childrenNode = node->mChildren[i];
			TraverseHierarchy(childrenNode, node);
		}
	}
}