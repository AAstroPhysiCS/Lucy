#include "lypch.h"
#include "Mesh.h"

#include "../Core/Timer.h"
#include "Renderer.h"

namespace Lucy {

	constexpr static uint32_t ASSIMP_FLAGS = aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals |
		aiProcess_FixInfacingNormals |
		aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ImproveCacheLocality |
		aiProcess_LimitBoneWeights |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_ValidateDataStructure |
		aiProcess_Triangulate |
		//aiProcess_PreTransformVertices | (animations won't work, if you enable this)
		aiProcess_SplitLargeMeshes |
		aiProcess_OptimizeMeshes;

	static void IncreaseMeshCount(Mesh* m) {
		if (MESH_ID_COUNT_X <= 255) {
			MESH_ID_COUNT_X++;
		} else {
			MESH_ID_COUNT_X = 0;
			if (MESH_ID_COUNT_Y <= 255) {
				MESH_ID_COUNT_Y++;
			} else {
				MESH_ID_COUNT_Y = 0;
				if (MESH_ID_COUNT_Z <= 255) {
					MESH_ID_COUNT_Z++;
				}
			}
		}
		m->m_PixelValue = glm::vec3(MESH_ID_COUNT_X, MESH_ID_COUNT_Y, MESH_ID_COUNT_Z);
	}

	Mesh::Mesh(const std::string& path)
		: m_Path(path) {
		ScopedTimer scopedTimer;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, ASSIMP_FLAGS);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
			LUCY_CRITICAL("Mesh could not be imported!");
			return;
		}

		m_Name = scene->mRootNode->mName.C_Str();

		LoadData(scene);
		TraverseHierarchy(scene->mRootNode, nullptr);

		//Getting the size of the buffer
		for (Submesh& submesh : m_Submeshes) {
			m_MetadataInfo.TotalIndicesSize += submesh.IndexCount;
			m_MetadataInfo.TotalVerticesSize += submesh.VertexCount;
		}

		m_IndexBuffer = IndexBuffer::Create(m_MetadataInfo.TotalIndicesSize);
		m_VertexBuffer = VertexBuffer::Create(m_MetadataInfo.TotalVerticesSize * 17);
		IncreaseMeshCount(this);

		uint32_t from = 0;
		for (uint32_t i = 0; i < m_Submeshes.size(); i++) {
			Submesh& submesh = m_Submeshes[i];
			auto& faces = submesh.Faces;
			m_IndexBuffer->SetData(faces, from);
			from += faces.size();
		}

		from = 0;

		for (Submesh& submesh : m_Submeshes) {
			auto& vertices = submesh.Vertices;
			auto& textureCoords = submesh.TextureCoords;
			auto& normals = submesh.Normals;
			auto& tangents = submesh.Tangents;
			auto& biTangents = submesh.BiTangents;

			for (uint32_t i = 0; i < submesh.VertexCount; i++) {
				std::vector<float> vertex = {
					vertices[i].x,
					vertices[i].y,
					vertices[i].z,

					textureCoords[i].x,
					textureCoords[i].y,

					(float)MESH_ID_COUNT_X,
					(float)MESH_ID_COUNT_Y,
					(float)MESH_ID_COUNT_Z,

					normals[i].x,
					normals[i].y,
					normals[i].z,

					tangents[i].x,
					tangents[i].y,
					tangents[i].z,

					biTangents[i].x,
					biTangents[i].y,
					biTangents[i].z
				};
				m_VertexBuffer->SetData(vertex, from);
				from += vertex.size();
			}
		}

		m_VertexBuffer->LoadToGPU();
		m_IndexBuffer->LoadToGPU();
	}

	Ref<Mesh> Mesh::Create(const std::string& path) {
		return Memory::CreateRef<Mesh>(path);
	}

	void Mesh::LoadData(const aiScene* scene) {
		aiMesh** meshes = scene->mMeshes;
		uint32_t meshCount = scene->mNumMeshes;

		uint32_t baseVertexCount = 0;
		uint32_t baseIndexCount = 0;

		m_Materials.resize(scene->mNumMaterials);

		for (uint32_t i = 0; i < meshCount; i++) {
			aiMesh* mesh = meshes[i];
			Submesh submesh;
			submesh.BaseVertexCount = baseVertexCount;
			submesh.BaseIndexCount = baseIndexCount;
			submesh.VertexCount = mesh->mNumVertices;
			submesh.IndexCount = mesh->mNumFaces * 3;
			submesh.MaterialIndex = mesh->mMaterialIndex;

			baseVertexCount += submesh.VertexCount;
			baseIndexCount += submesh.IndexCount;

			LoadMaterials(scene, mesh);

			uint32_t sizeVertices = submesh.VertexCount;

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
				submesh.TextureCoords.reserve(sizeVertices);
				for (uint32_t j = 0; j < sizeVertices; j++) {
					submesh.TextureCoords.emplace_back(textureCoords[j].x, textureCoords[j].y);
				}
			}

			if (mesh->HasTangentsAndBitangents()) {
				aiVector3D* tangents = mesh->mTangents;
				submesh.Tangents.resize(sizeVertices);
				memcpy(submesh.Tangents.data(), tangents, sizeVertices * sizeof(aiVector3D));

				aiVector3D* biTangents = mesh->mBitangents;
				submesh.BiTangents.resize(sizeVertices);
				memcpy(submesh.BiTangents.data(), biTangents, sizeVertices * sizeof(aiVector3D));
			}

			if (mesh->HasFaces()) {
				submesh.Faces.reserve(submesh.IndexCount);
				for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
					aiFace aiFace = mesh->mFaces[j];
					for (uint32_t k = 0; k < aiFace.mNumIndices; k++) {
						submesh.Faces.emplace_back(aiFace.mIndices[k]);
					}
				}
			}

			//TODO: Animation

			m_Submeshes.push_back(submesh);
		}
	}

	void Mesh::LoadMaterials(const aiScene* scene, const aiMesh* mesh) {
		m_Materials[mesh->mMaterialIndex] = Material::Create(Renderer::GetShaderLibrary().GetShader("LucyPBR"), scene->mMaterials[mesh->mMaterialIndex], mesh->mName.data, m_Path);
	}

	void Mesh::TraverseHierarchy(const aiNode* node, const aiNode* rootNode) {
		aiMatrix4x4 transform = node->mTransformation;
		if (rootNode) {
			transform *= rootNode->mTransformation;
		}

		for (uint32_t i = 0; i < node->mNumMeshes; i++) {
			Submesh& submesh = m_Submeshes[node->mMeshes[i]];
			submesh.Transform = *(glm::mat4*)&transform;
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++) {
			aiNode* childrenNode = node->mChildren[i];
			TraverseHierarchy(childrenNode, node);
		}
	}

	void Mesh::Destroy() {
		for (Ref<Material> material : m_Materials)
			material->Destroy();

		m_VertexBuffer->DestroyHandle();
		m_IndexBuffer->DestroyHandle();
	}
}