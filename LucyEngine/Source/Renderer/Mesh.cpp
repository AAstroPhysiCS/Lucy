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
		const uint32_t id = s_NextMeshID.fetch_add(1, std::memory_order_relaxed);

		LUCY_ASSERT(id <= 0x00FFFFFFu, "Maximum mesh ID count exceeded.");

		return {
			static_cast<float>(id & 0xFFu),
			static_cast<float>((id >> 8u) & 0xFFu),
			static_cast<float>((id >> 16u) & 0xFFu)
		};
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

	void Mesh::Load(const Ref<RenderDevice>& device, const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
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

		static constexpr uint32_t VERTEX_SIZE = 17; // position(3) + meshID(3) + uv(2) + normal(3) + tangent(3) + bitangent(3)

		std::vector<float> packedVertices;
		std::vector<uint32_t> packedIndices;

		packedVertices.resize(static_cast<size_t>(m_MetadataInfo.TotalVerticesSize) * VERTEX_SIZE);
		packedIndices.resize(static_cast<size_t>(m_MetadataInfo.TotalIndicesSize));

		for (const Submesh& submesh : m_Submeshes) {
			if (!submesh.Faces.empty())
				memcpy(packedIndices.data() + submesh.BaseIndexCount, submesh.Faces.data(), submesh.Faces.size() * sizeof(uint32_t));

			size_t dstFloatOffset = static_cast<size_t>(submesh.BaseVertexCount) * VERTEX_SIZE;

			for (uint32_t i = 0; i < submesh.VertexCount; i++) {
				glm::vec2 uv = { 0.0f, 0.0f };
				if (!submesh.TextureCoords.empty())
					uv = { submesh.TextureCoords[i].x, submesh.TextureCoords[i].y };

				glm::vec3 n = { 0.0f, 0.0f, 0.0f };
				if (!submesh.Normals.empty())
					n = { submesh.Normals[i].x, submesh.Normals[i].y, submesh.Normals[i].z };

				glm::vec3 t = { 0.0f, 0.0f, 0.0f };
				if (!submesh.Tangents.empty())
					t = { submesh.Tangents[i].x, submesh.Tangents[i].y, submesh.Tangents[i].z };

				glm::vec3 b = { 0.0f, 0.0f, 0.0f };
				if (!submesh.BiTangents.empty())
					b = { submesh.BiTangents[i].x, submesh.BiTangents[i].y, submesh.BiTangents[i].z };

				const auto& p = submesh.Vertices[i];
				float* dst = packedVertices.data() + dstFloatOffset;

				dst[0] = p.x;
				dst[1] = p.y;
				dst[2] = p.z;

				dst[3] = meshID.x;
				dst[4] = meshID.y;
				dst[5] = meshID.z;

				dst[6] = uv.x;
				dst[7] = uv.y;

				dst[8] = n.x;
				dst[9] = n.y;
				dst[10] = n.z;

				dst[11] = t.x;
				dst[12] = t.y;
				dst[13] = t.z;

				dst[14] = b.x;
				dst[15] = b.y;
				dst[16] = b.z;

				dstFloatOffset += VERTEX_SIZE;
			}
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

			if (mesh->HasPositions()) {
				submesh.Vertices.resize(vertexCount);
				memcpy(submesh.Vertices.data(), mesh->mVertices, vertexCount * sizeof(aiVector3D));
			}

			if (mesh->HasNormals()) {
				submesh.Normals.resize(vertexCount);
				memcpy(submesh.Normals.data(), mesh->mNormals, vertexCount * sizeof(aiVector3D));
			}

			if (mesh->HasTextureCoords(0)) {
				submesh.TextureCoords.resize(vertexCount);
				const aiVector3D* textureCoords = mesh->mTextureCoords[0];
				for (uint32_t j = 0; j < vertexCount; j++) {
					submesh.TextureCoords[j] = { textureCoords[j].x, textureCoords[j].y };
				}
			}

			if (mesh->HasTangentsAndBitangents()) {
				submesh.Tangents.resize(vertexCount);
				submesh.BiTangents.resize(vertexCount);

				memcpy(submesh.Tangents.data(), mesh->mTangents, vertexCount * sizeof(aiVector3D));
				memcpy(submesh.BiTangents.data(), mesh->mBitangents, vertexCount * sizeof(aiVector3D));
			}

			if (mesh->HasFaces()) {
				submesh.Faces.resize(submesh.IndexCount);

				uint32_t dst = 0;
				for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
					const aiFace& face = mesh->mFaces[j];
					LUCY_ASSERT(face.mNumIndices == 3, "Mesh is expected to be triangulated.");
					submesh.Faces[dst++] = face.mIndices[0];
					submesh.Faces[dst++] = face.mIndices[1];
					submesh.Faces[dst++] = face.mIndices[2];
				}
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