#include "lypch.h"
#include "Mesh.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "Memory/Buffer/VertexBuffer.h"
#include "Memory/Buffer/IndexBuffer.h"

#include "../Core/Application.h"

#include "Renderer.h"

#include "Core/Timer.h"

#include <unordered_map>

namespace Lucy {

	constexpr static inline uint32_t ASSIMP_FLAGS = aiProcess_FlipUVs | aiProcessPreset_TargetRealtime_MaxQuality;

	[[nodiscard]] glm::vec3 AllocateMeshID() {
		const uint32_t id = Mesh::s_NextMeshID.fetch_add(1, std::memory_order_relaxed);

		LUCY_ASSERT(id <= 0x00FFFFFFu, "Maximum mesh ID count exceeded.");

		return {
			static_cast<float>(id & 0xFFu),
			static_cast<float>((id >> 8u) & 0xFFu),
			static_cast<float>((id >> 16u) & 0xFFu)
		};
	}

	Ref<Mesh> Mesh::Create(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
		LUCY_ASSERT(vertices.size() % 3 == 0, "Position array must contain complete vec3 values.");

		const size_t vertexCount = vertices.size() / 3;
		std::vector<Vertex> convertedVertices(vertexCount);

		for (size_t i = 0; i < vertexCount; i++) {
			const size_t sourceIndex = i * 3;
			convertedVertices[i].Position = { vertices[sourceIndex + 0], vertices[sourceIndex + 1], vertices[sourceIndex + 2] };
		}

		return Memory::CreateRef<Mesh>(convertedVertices, indices);
	}

	Ref<Mesh> Mesh::Create(const std::string& path) {
		return Memory::CreateRef<Mesh>(path);
	}

	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
		Renderer::EnqueueToRenderCommandQueue([=](Ref<RenderDevice>& device) {
			Load(device, vertices, indices);
		});
	}

	Mesh::Mesh(const std::string& path)
		: m_Path(path) {
		Load();
	}

	void Mesh::Load(const Ref<RenderDevice>& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
		m_VertexBufferHandle = device->CreateVertexBuffer(vertices.size() * sizeof(Vertex));
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

		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
			LUCY_CRITICAL("Mesh could not be imported!");
			LUCY_CRITICAL(importer.GetErrorString());
			return;
		}

		m_Name = scene->mRootNode->mName.C_Str();
		m_MetadataInfo = {};
		m_Submeshes.clear();

		LoadProgram(scene);
		TraverseHierarchy(scene->mRootNode, glm::mat4(1.0f));
		m_MeshID = AllocateMeshID();

		const glm::vec3 meshID = m_MeshID;

		std::vector<Vertex> packedVertices(m_MetadataInfo.TotalVerticesSize);
		std::vector<uint32_t> packedIndices(m_MetadataInfo.TotalIndicesSize);

		for (const Submesh& submesh : m_Submeshes) {
			memcpy(&packedVertices[submesh.BaseVertexCount], submesh.Vertices.data(), submesh.Vertices.size() * sizeof(Vertex));
			memcpy(&packedIndices[submesh.BaseIndexCount], submesh.Indices.data(), submesh.Indices.size() * sizeof(uint32_t));
		}

		Renderer::EnqueueToRenderCommandQueue([this, packedVertices = std::move(packedVertices), packedIndices = std::move(packedIndices)](const Ref<RenderDevice>& device) {
			Load(device, packedVertices, packedIndices);
		});
	}

	void Mesh::LoadProgram(const aiScene* scene) {
		const auto& taskScheduler = Application::GetTaskScheduler();

		ScopedTimer scopedTimer(std::format("{0} data parsing", m_Name));

		aiMesh** meshes = scene->mMeshes;
		const uint32_t meshCount = scene->mNumMeshes;

		m_Submeshes.resize(meshCount);

		taskScheduler->ScheduleBatch(TaskScheduler::Launch::Async, TaskPriority::High, [=](const TaskArgs& args, const TaskBatchArgs&) {
			const uint32_t index = static_cast<uint32_t>(args.TaskIndex);

			aiMesh* mesh = meshes[index];
			Submesh& submesh = m_Submeshes[index];

			submesh.VertexCount = mesh->mNumVertices;
			submesh.IndexCount = mesh->mNumFaces * 3;

			const uint32_t vertexCount = submesh.VertexCount;

			submesh.Vertices.resize(submesh.VertexCount);
			submesh.Indices.resize(submesh.IndexCount);

			aiVector3D* positions = mesh->HasPositions() ? mesh->mVertices : nullptr;
			aiVector3D* normals = mesh->HasNormals() ? mesh->mNormals : nullptr;
			aiVector3D* textureCoords = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0] : nullptr;

			bool hasTangents = mesh->HasTangentsAndBitangents();
			aiVector3D* tangents = hasTangents ? mesh->mTangents : nullptr;
			aiVector3D* bitangents = hasTangents ? mesh->	mBitangents : nullptr;

			for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
				Vertex& vertex = submesh.Vertices[vertexIndex];
				vertex.MeshID = m_MeshID;

				if (positions) {
					aiVector3D& position = positions[vertexIndex];
					vertex.Position = { position.x, position.y, position.z };
				}

				if (normals) {
					aiVector3D& normal = normals[vertexIndex];
					vertex.Normal = { normal.x, normal.y, normal.z };
				}

				if (textureCoords) {
					aiVector3D& textureCoordinate = textureCoords[vertexIndex];
					vertex.TexCoords = { textureCoordinate.x, textureCoordinate.y };
				}

				if (hasTangents) {
					aiVector3D& tangent = tangents[vertexIndex];
					aiVector3D& bitangent = bitangents[vertexIndex];

					vertex.Tangent = { tangent.x, tangent.y, tangent.z };
					vertex.Bitangent = { bitangent.x, bitangent.y, bitangent.z };
				}
			}

			uint32_t* destination = submesh.Indices.data();

			for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
				aiFace& face = mesh->mFaces[faceIndex];
				LUCY_ASSERT(face.mNumIndices == 3, "Mesh is expected to be triangulated.");

				destination[0] = face.mIndices[0];
				destination[1] = face.mIndices[1];
				destination[2] = face.mIndices[2];

				destination += 3;
			}
		}, meshCount, 1);

		taskScheduler->WaitForAllTasks();

		uint32_t runningVertexOffset = 0;
		uint32_t runningIndexOffset = 0;

		for (uint32_t i = 0; i < meshCount; i++) {
			Submesh& submesh = m_Submeshes[i];

			submesh.BaseVertexCount = runningVertexOffset;
			submesh.BaseIndexCount = runningIndexOffset;

			runningVertexOffset += submesh.VertexCount;
			runningIndexOffset += submesh.IndexCount;
		}

		m_MetadataInfo.TotalVerticesSize = runningVertexOffset;
		m_MetadataInfo.TotalIndicesSize = runningIndexOffset;

		for (uint32_t i = 0; i < meshCount; i++) {
			aiMesh* mesh = meshes[i];
			m_Submeshes[i].MaterialID = Renderer::GetMaterialManager()->CreateMaterialByPath(MaterialType::PBR, scene->mMaterials[mesh->mMaterialIndex], m_Path);
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
			aiNode* childNode = node->mChildren[i];
			TraverseHierarchy(childNode, transformed);
		}
	}

	void Mesh::Destroy() {
		for (Submesh& submesh : m_Submeshes)
			Renderer::GetMaterialManager()->RTDestroyMaterial(submesh.MaterialID);

		Renderer::EnqueueResourceDestroy(m_VertexBufferHandle);
		Renderer::EnqueueResourceDestroy(m_IndexBufferHandle);
	}
}