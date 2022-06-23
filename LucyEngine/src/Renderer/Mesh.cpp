#include "lypch.h"
#include "Mesh.h"

#include "../Core/Timer.h"
#include "Renderer.h"
#include "Shader/Shader.h"

#include "RenderPass.h"
#include "Context/OpenGLPipeline.h"

#include "VulkanMesh.h"
#include "OpenGLMesh.h"

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

		uint32_t totalVertexSize = 0;
		LoadData(scene, totalVertexSize);
		TraverseHierarchy(scene->mRootNode, nullptr);

		m_IndexBuffer = IndexBuffer::Create();
		for (Submesh& submesh : m_Submeshes) {
			m_IndexBuffer->Append(submesh.Faces);
		}

		m_VertexBuffer = VertexBuffer::Create(totalVertexSize * 17);
		IncreaseMeshCount(this);

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

					(float)MESH_ID_COUNT_X,
					(float)MESH_ID_COUNT_Y,
					(float)MESH_ID_COUNT_Z,

					textureCoords[i].x,
					textureCoords[i].y,

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
				m_VertexBuffer->SetData(vertex);
			}
		}
	}

	Mesh::~Mesh() {
		m_VertexBuffer->DestroyHandle();
		m_IndexBuffer->DestroyHandle();
	}

	Ref<Mesh> Mesh::Create(const std::string& path) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLMesh>(path);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanMesh>(path);
				break;
		}
		return nullptr;
	}

	void Mesh::LoadData(const aiScene* scene, uint32_t& totalVertexSize) {
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

			totalVertexSize += submesh.VertexCount;

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
}