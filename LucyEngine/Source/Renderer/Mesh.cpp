#include "lypch.h"
#include "Mesh.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "Memory/Buffer/VertexBuffer.h"
#include "Memory/Buffer/IndexBuffer.h"

#include "Renderer.h"

#include "Core/Timer.h"

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
		m->m_MeshID = glm::vec3(MESH_ID_COUNT_X, MESH_ID_COUNT_Y, MESH_ID_COUNT_Z);
	}

	Ref<Mesh> Mesh::Create(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
		return Memory::CreateRef<Mesh>(vertices, indices);
	}

	Ref<Mesh> Mesh::Create(const std::string& path) {
		return Memory::CreateRef<Mesh>(path);
	}

	Mesh::Mesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
		Renderer::EnqueueToRenderCommandQueue([=](Ref<RenderDevice>& device) {
			Load(device, vertices, indices);
		});
	}

	Mesh::Mesh(const std::string& path)
		: m_Path(path) {
		Load();
	}

	void Mesh::Load(Ref<RenderDevice>& device, const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
		m_VertexBufferHandle = device->CreateVertexBuffer(vertices.size());
		m_IndexBufferHandle = device->CreateIndexBuffer(indices.size());

		const auto& vertexBuffer = Renderer::AccessResource<VertexBuffer>(m_VertexBufferHandle);
		const auto& indexBuffer = Renderer::AccessResource<IndexBuffer>(m_IndexBufferHandle);

		vertexBuffer->SetData(vertices);
		indexBuffer->SetData(indices);

		vertexBuffer->RTLoadToDevice();
		indexBuffer->RTLoadToDevice();
	}

	void Mesh::Load() {
		ScopedTimer scopedTimer("Mesh import");

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(m_Path, ASSIMP_FLAGS);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			LUCY_CRITICAL("Mesh could not be imported!");
			LUCY_CRITICAL(importer.GetErrorString());
			return;
		}

		m_Name = scene->mRootNode->mName.C_Str();

		LoadData(scene);
		TraverseHierarchy(scene->mRootNode, glm::mat4(1.0f));

		IncreaseMeshCount(this);

		//Getting the size of the buffer
		for (const Submesh& submesh : m_Submeshes) {
			m_MetadataInfo.TotalIndicesSize += submesh.IndexCount;
			m_MetadataInfo.TotalVerticesSize += submesh.VertexCount;
		}

		Renderer::EnqueueToRenderCommandQueue([=](const Ref<RenderDevice>& device) {
			m_VertexBufferHandle = device->CreateVertexBuffer(m_MetadataInfo.TotalVerticesSize * 17LL);
			m_IndexBufferHandle = device->CreateIndexBuffer(m_MetadataInfo.TotalIndicesSize);

			const auto& vertexBuffer = Renderer::AccessResource<VertexBuffer>(m_VertexBufferHandle);
			const auto& indexBuffer = Renderer::AccessResource<IndexBuffer>(m_IndexBufferHandle);

			size_t from = 0;
			for (uint32_t i = 0; i < m_Submeshes.size(); i++) {
				const Submesh& submesh = m_Submeshes[i];
				auto& faces = submesh.Faces;
				indexBuffer->SetData(faces, from);
				from += faces.size();
			}

			from = 0;

			for (Submesh& submesh : m_Submeshes) {
				auto& submeshVertices = submesh.Vertices;
				auto& submeshTextureCoords = submesh.TextureCoords;
				auto& submeshNormals = submesh.Normals;
				auto& submeshTangents = submesh.Tangents;
				auto& submeshBiTangents = submesh.BiTangents;

				for (uint32_t i = 0; i < submesh.VertexCount; i++) {

					glm::vec2 textureCoords = { 0.0f, 0.0f };
					if (!submeshTextureCoords.empty())
						textureCoords = { submeshTextureCoords[i].x, submeshTextureCoords[i].y };

					glm::vec3 normals = { 0.0f, 0.0f, 0.0f };
					if (!submeshNormals.empty())
						normals = { submeshNormals[i].x, submeshNormals[i].y, submeshNormals[i].z };

					glm::vec3 tangents = { 0.0f, 0.0f, 0.0f };
					if (!submeshTangents.empty())
						tangents = { submeshTangents[i].x, submeshTangents[i].y, submeshTangents[i].z };

					glm::vec3 biTangents = { 0.0f, 0.0f, 0.0f };
					if (!submeshBiTangents.empty())
						biTangents = { submeshBiTangents[i].x, submeshBiTangents[i].y, submeshBiTangents[i].z };

					std::vector<float> vertex = {
						submeshVertices[i].x,
						submeshVertices[i].y,
						submeshVertices[i].z,

						(float)MESH_ID_COUNT_X,
						(float)MESH_ID_COUNT_Y,
						(float)MESH_ID_COUNT_Z,

						textureCoords.x,
						textureCoords.y,

						normals.x,
						normals.y,
						normals.z,

						tangents.x,
						tangents.y,
						tangents.z,

						biTangents.x,
						biTangents.y,
						biTangents.z
					};
					vertexBuffer->SetData(vertex, from);
					from += vertex.size();
				}
			}
				
			vertexBuffer->RTLoadToDevice();
			indexBuffer->RTLoadToDevice();
		});
	}

	void Mesh::LoadData(const aiScene* scene) {
		ScopedTimer scopedTimer(std::format("{0} data parsing", m_Name));

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

			submesh.MaterialID = Renderer::GetMaterialManager()->CreateMaterialByPath(MaterialType::PBR, 
				scene->mMaterials[mesh->mMaterialIndex], m_Path);
			m_Submeshes.push_back(submesh);
		}
	}

	void Mesh::TraverseHierarchy(const aiNode* node, const glm::mat4& parentTransform) {
		glm::mat4 localTransform = *(glm::mat4*)&node->mTransformation;
		glm::mat4 transformed = parentTransform * localTransform;

		for (uint32_t i = 0; i < node->mNumMeshes; i++) {
			Submesh& submesh = m_Submeshes[node->mMeshes[i]];
			submesh.Transform = *(glm::mat4*)&transformed;
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++) {
			aiNode* childrenNode = node->mChildren[i];
			TraverseHierarchy(childrenNode, transformed);
		}
	}

	void Mesh::Destroy() {
		for (Submesh& submesh : m_Submeshes)
			Renderer::GetMaterialManager()->RTDestroyMaterial(submesh.MaterialID);
		Renderer::EnqueueResourceDestroy(m_VertexBufferHandle);
		Renderer::EnqueueResourceDestroy(m_IndexBufferHandle);
	}
}