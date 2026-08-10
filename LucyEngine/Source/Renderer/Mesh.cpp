#include "lypch.h"
#include "Mesh.h"

#include "meshoptimizer.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "Memory/Buffer/VertexBuffer.h"
#include "Memory/Buffer/IndexBuffer.h"

#include "../Core/Application.h"

#include "Renderer.h"
#include "Renderer/Device/RenderDeviceScene.h"

#include "Material/MaterialManager.h"

#include "Core/Timer.h"

namespace Lucy {

	//constexpr static inline uint32_t ASSIMP_FLAGS = aiProcess_FlipUVs | aiProcessPreset_TargetRealtime_Quality;

	constexpr static uint32_t ASSIMP_FLAGS = aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals |
		aiProcess_FixInfacingNormals |
		aiProcess_FlipUVs |
		aiProcess_LimitBoneWeights |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_ValidateDataStructure |
		aiProcess_Triangulate |
		//aiProcess_PreTransformVertices | (animations won't work, if you enable this)
		aiProcess_SplitLargeMeshes |
		aiProcess_OptimizeMeshes;

	constexpr static float MESHOPT_OVERDRAW_THRESHOLD = 1.05f;

	[[nodiscard]] glm::vec3 AllocateMeshID() {
		const uint32_t id = Mesh::s_NextMeshID.fetch_add(1, std::memory_order_relaxed);

		LUCY_ASSERT(id <= 0x00FFFFFFu, "Maximum mesh ID count exceeded.");

		return {
			static_cast<float>(id & 0xFFu),
			static_cast<float>((id >> 8u) & 0xFFu),
			static_cast<float>((id >> 16u) & 0xFFu)
		};
	}

	Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
		m_MetadataInfo.TotalVerticesSize = static_cast<uint32_t>(vertices.size());
		m_MetadataInfo.TotalIndicesSize = static_cast<uint32_t>(indices.size());

		Renderer::EnqueueToRenderCommandQueue([this, vertices = std::move(vertices), indices = std::move(indices)](const Ref<RenderDevice>& device) mutable {
			Load(device, vertices, indices);
		});
	}

	Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices) {
		m_MetadataInfo.TotalVerticesSize = static_cast<uint32_t>(vertices.size());
		m_MetadataInfo.TotalIndicesSize = static_cast<uint32_t>(indices.size());

		Renderer::EnqueueToRenderCommandQueue([this, vertices = std::move(vertices), indices = std::move(indices)](const Ref<RenderDevice>& device) mutable {
			Load(device, vertices, indices);
		});
	}

	Mesh::Mesh(const std::string& path)
		: m_Path(path) {
		Load();
	}

	void Mesh::Load(const Ref<RenderDevice>& device, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
		m_RenderDeviceMeshHandle = device->GetScene()->RegisterMesh(vertices, indices, m_Submeshes);
		//need to submit since registermesh also submits
		Renderer::EnqueueToRenderCommandQueue([this](const auto& device) {
			ReleaseCPUData();
		});
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

		m_MeshID = AllocateMeshID();
		LoadProgram(scene);
		TraverseHierarchy(scene->mRootNode, glm::mat4(1.0f));

		std::vector<Vertex> packedVertices(m_MetadataInfo.TotalVerticesSize);
		std::vector<uint32_t> packedIndices(m_MetadataInfo.TotalMeshletIndicesSize);

		for (const Submesh& submesh : m_Submeshes) {
			memcpy(&packedVertices[submesh.BaseVertexCount], submesh.Vertices.data(), submesh.Vertices.size() * sizeof(Vertex));
			memcpy(&packedIndices[submesh.BaseMeshletIndexCount], submesh.MeshletIndices.data(), submesh.MeshletIndices.size() * sizeof(uint32_t));
		}

		Renderer::EnqueueToRenderCommandQueue([this, packedVertices = std::move(packedVertices), packedIndices = std::move(packedIndices)](const Ref<RenderDevice>& device) mutable {
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
			aiVector3D* bitangents = hasTangents ? mesh->mBitangents : nullptr;

			for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
				Vertex& vertex = submesh.Vertices[vertexIndex];
				vertex = {};

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

			OptimizeMeshData(submesh.Vertices, submesh.Indices);
			BuildLODs(submesh);

			submesh.VertexCount = static_cast<uint32_t>(submesh.Vertices.size());
			submesh.IndexCount = static_cast<uint32_t>(submesh.Indices.size());
		}, meshCount, 1);

		taskScheduler->WaitForAllTasks();

		uint32_t runningVertexOffset = 0;
		uint32_t runningIndexOffset = 0;
		uint32_t runningMeshletOffset = 0;
		uint32_t runningMeshletIndexOffset = 0;
		uint32_t runningMeshletVertexOffset = 0;
		uint32_t runningMeshletTriangleOffset = 0;

		for (uint32_t i = 0; i < meshCount; i++) {
			Submesh& submesh = m_Submeshes[i];

			submesh.BaseVertexCount = runningVertexOffset;
			submesh.BaseIndexCount = runningIndexOffset;
			submesh.BaseMeshletCount = runningMeshletOffset;
			submesh.BaseMeshletIndexCount = runningMeshletIndexOffset;
			submesh.BaseMeshletVertexCount = runningMeshletVertexOffset;
			submesh.BaseMeshletTriangleCount = runningMeshletTriangleOffset;

			runningVertexOffset += submesh.VertexCount;
			runningIndexOffset += submesh.IndexCount;
			runningMeshletOffset += static_cast<uint32_t>(submesh.Meshlets.size());
			runningMeshletIndexOffset += static_cast<uint32_t>(submesh.MeshletIndices.size());
			runningMeshletVertexOffset += static_cast<uint32_t>(submesh.MeshletVertices.size());
			runningMeshletTriangleOffset += static_cast<uint32_t>(submesh.MeshletTriangles.size());
		}

		m_MetadataInfo.TotalVerticesSize = runningVertexOffset;
		m_MetadataInfo.TotalIndicesSize = runningIndexOffset;
		m_MetadataInfo.TotalMeshletsSize = runningMeshletOffset;
		m_MetadataInfo.TotalMeshletIndicesSize = runningMeshletIndexOffset;
		m_MetadataInfo.TotalMeshletVerticesSize = runningMeshletVertexOffset;
		m_MetadataInfo.TotalMeshletTrianglesSize = runningMeshletTriangleOffset;

		const auto& materialManager = Renderer::GetMaterialManager();
		for (uint32_t i = 0; i < meshCount; i++) {
			aiMesh* mesh = meshes[i];
			m_Submeshes[i].MaterialID = materialManager->CreateMaterialByPath(MaterialType::PBR, scene->mMaterials[mesh->mMaterialIndex], m_Path);
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
	/* 
	* NOTE FOR FUTURE:
	* these optimizations reorder triangles. That is correct for ordinary opaque geometry, 
	* but order-dependent alpha-blended submeshes should not use this triangle-reordering path unless 
	* transparency is handled through sorting or order-independent transparency
	*/
	void Mesh::OptimizeMeshData(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
		meshopt_Stream vertexStreams[] = {
			{ &vertices[0].Position.x, sizeof(float) * 3, sizeof(Vertex) },
			{ &vertices[0].MeshID.x, sizeof(float) * 3, sizeof(Vertex) },
			{ &vertices[0].TexCoords.x, sizeof(float) * 2, sizeof(Vertex) },
			{ &vertices[0].Normal.x, sizeof(float) * 3, sizeof(Vertex) },
			{ &vertices[0].Tangent.x, sizeof(float) * 3, sizeof(Vertex) },
			{ &vertices[0].Bitangent.x, sizeof(float) * 3, sizeof(Vertex) }
		};

		std::vector<uint32_t> vertexRemap(vertices.size());

		size_t optimizedVertexCount = meshopt_generateVertexRemapMulti(vertexRemap.data(), indices.data(), indices.size(),
			vertices.size(), vertexStreams, sizeof(vertexStreams) / sizeof(vertexStreams[0]));

		std::vector<Vertex> optimizedVertices(optimizedVertexCount);
		std::vector<uint32_t> optimizedIndices(indices.size());

		meshopt_remapVertexBuffer(optimizedVertices.data(), vertices.data(), vertices.size(), sizeof(Vertex), vertexRemap.data());
		meshopt_remapIndexBuffer(optimizedIndices.data(), indices.data(), indices.size(), vertexRemap.data());
		meshopt_optimizeVertexCache(optimizedIndices.data(), optimizedIndices.data(), optimizedIndices.size(), optimizedVertexCount);
		meshopt_optimizeOverdraw(optimizedIndices.data(), optimizedIndices.data(), optimizedIndices.size(),
			&optimizedVertices[0].Position.x, optimizedVertexCount, sizeof(Vertex), MESHOPT_OVERDRAW_THRESHOLD);

		size_t finalVertexCount = meshopt_optimizeVertexFetch(optimizedVertices.data(), optimizedIndices.data(), optimizedIndices.size(),
			optimizedVertices.data(), optimizedVertexCount, sizeof(Vertex));

		optimizedVertices.resize(finalVertexCount);

		vertices = std::move(optimizedVertices);
		indices = std::move(optimizedIndices);
	}

	void Mesh::BuildLODs(Submesh& submesh) {
		submesh.LODs.reserve(MESH_LOD_COUNT);

		if (submesh.Vertices.empty() || submesh.Indices.empty()) {
			submesh.LODs.resize(MESH_LOD_COUNT);
			submesh.MeshletCount = 0;
			return;
		}

		float errorScale = meshopt_simplifyScale(&submesh.Vertices[0].Position.x, submesh.Vertices.size(), sizeof(Vertex));

		const std::vector<uint32_t>& baseIndices = submesh.Indices;

		for (uint32_t lodIndex = 0; lodIndex < MESH_LOD_COUNT; lodIndex++) {
			std::vector<uint32_t> lodIndices;
			float lodError = 0.0f;

			if (lodIndex == 0) {
				lodIndices = baseIndices;
			} else {
				size_t targetIndexCount = static_cast<size_t>(baseIndices.size() * MESH_LOD_RATIOS[lodIndex]);
				targetIndexCount = (targetIndexCount / 3) * 3;
				targetIndexCount = std::max<size_t>(targetIndexCount, 3);

				lodIndices.resize(baseIndices.size());

				const size_t simplifiedIndexCount = meshopt_simplify(lodIndices.data(), baseIndices.data(), baseIndices.size(),
					&submesh.Vertices[0].Position.x, submesh.Vertices.size(), sizeof(Vertex), targetIndexCount,
					MESH_LOD_TARGET_ERROR, 0, &lodError);

				if (simplifiedIndexCount < 3) {
					lodIndices = baseIndices;
					lodError = 0.0f;
				} else {
					lodIndices.resize(simplifiedIndexCount);
				}

				meshopt_optimizeVertexCache(lodIndices.data(), lodIndices.data(), lodIndices.size(), submesh.Vertices.size());
			}

			BuildMeshlets(submesh, lodIndices, lodError * errorScale, lodIndex);
		}

		submesh.MeshletCount = static_cast<uint32_t>(submesh.Meshlets.size());
	}

	void Mesh::BuildMeshlets(Submesh& submesh, const std::vector<uint32_t>& indices, float lodError, uint32_t lodIndex) {
		SubmeshLOD lod{};
		lod.FirstMeshlet = static_cast<uint32_t>(submesh.Meshlets.size());
		lod.FirstMeshletIndex = static_cast<uint32_t>(submesh.MeshletIndices.size());
		lod.Error = lodError;

		if (submesh.Vertices.empty() || indices.empty()) {
			submesh.LODs.push_back(lod);
			return;
		}

		size_t maxMeshletCount = meshopt_buildMeshletsBound(indices.size(), MESHLET_MAX_VERTICES, MESHLET_MIN_TRIANGLES);

		std::vector<meshopt_Meshlet> generatedMeshlets(maxMeshletCount);
		submesh.MeshletVertices.resize(indices.size());
		submesh.MeshletTriangles.resize(indices.size());

		size_t meshletCount = meshopt_buildMeshletsFlex(generatedMeshlets.data(), submesh.MeshletVertices.data(), submesh.MeshletTriangles.data(),
			indices.data(), indices.size(), &submesh.Vertices[0].Position.x, submesh.Vertices.size(), sizeof(Vertex), 
			MESHLET_MAX_VERTICES, MESHLET_MIN_TRIANGLES, MESHLET_MAX_TRIANGLES, MESHLET_CONE_WEIGHT, 2.0f);

		size_t baseMeshletVertexOffset = submesh.MeshletVertices.size();
		size_t baseMeshletTriangleOffset = submesh.MeshletTriangles.size();

		generatedMeshlets.resize(meshletCount);
		submesh.Meshlets.reserve(meshletCount);

		if (generatedMeshlets.empty()) {
			submesh.MeshletVertices.clear();
			submesh.MeshletTriangles.clear();
			submesh.MeshletCount = 0;

			submesh.LODs.push_back(lod);
			return;
		}

		for (size_t meshletIndex = 0; meshletIndex < generatedMeshlets.size(); meshletIndex++) {
			const meshopt_Meshlet& generatedMeshlet = generatedMeshlets[meshletIndex];

			meshopt_optimizeMeshlet(submesh.MeshletVertices.data() + generatedMeshlet.vertex_offset, submesh.MeshletTriangles.data() + generatedMeshlet.triangle_offset,
				generatedMeshlet.triangle_count, generatedMeshlet.vertex_count);

			meshopt_Bounds bounds = meshopt_computeMeshletBounds(submesh.MeshletVertices.data() + generatedMeshlet.vertex_offset, 
				submesh.MeshletTriangles.data() + generatedMeshlet.triangle_offset, generatedMeshlet.triangle_count, &submesh.Vertices[0].Position.x,
				submesh.Vertices.size(), sizeof(Vertex));

			Meshlet meshlet{};
			meshlet.VertexCount = generatedMeshlet.vertex_count;
			meshlet.TriangleCount = generatedMeshlet.triangle_count;

			meshlet.FirstIndex = static_cast<uint32_t>(submesh.MeshletIndices.size());
			meshlet.IndexCount = generatedMeshlet.triangle_count * 3;

			for (uint32_t index = 0; index < meshlet.IndexCount; index++) {
				uint8_t meshletVertexIndex = submesh.MeshletTriangles[generatedMeshlet.triangle_offset + index];
				uint32_t submeshVertexIndex = submesh.MeshletVertices[generatedMeshlet.vertex_offset + meshletVertexIndex];

				submesh.MeshletIndices.push_back(submeshVertexIndex);
			}

			glm::vec3 minimum{ std::numeric_limits<float>::max() };
			glm::vec3 maximum{ std::numeric_limits<float>::lowest() };

			for (uint32_t i = 0; i < generatedMeshlet.vertex_count; i++) {
				const uint32_t submeshVertexIndex = submesh.MeshletVertices[generatedMeshlet.vertex_offset + i];
				const glm::vec3& position = submesh.Vertices[submeshVertexIndex].Position;

				minimum = glm::min(minimum, position);
				maximum = glm::max(maximum, position);
			}

			meshlet.AABBCenter = (minimum + maximum) * 0.5f;
			meshlet.AABBExtents = (maximum - minimum) * 0.5f;

			meshlet.BoundingSphere = { bounds.center[0], bounds.center[1], bounds.center[2], bounds.radius };
			meshlet.NormalCone = { bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2], bounds.cone_cutoff };

			submesh.Meshlets.push_back(meshlet);
		}

		const meshopt_Meshlet& lastMeshlet = generatedMeshlets.back();

		size_t usedMeshletVertexCount = static_cast<size_t>(lastMeshlet.vertex_offset) + static_cast<size_t>(lastMeshlet.vertex_count);
		size_t lastTriangleByteCount = (static_cast<size_t>(lastMeshlet.triangle_count) * 3 + 3) & ~size_t(3);
		size_t usedMeshletTriangleCount = static_cast<size_t>(lastMeshlet.triangle_offset) + lastTriangleByteCount;

		submesh.MeshletVertices.resize(usedMeshletVertexCount);
		submesh.MeshletTriangles.resize(usedMeshletTriangleCount);
		submesh.MeshletCount = static_cast<uint32_t>(meshletCount);

		lod.MeshletCount = static_cast<uint32_t>(meshletCount);
		lod.MeshletIndexCount = static_cast<uint32_t>(submesh.MeshletIndices.size()) - lod.FirstMeshletIndex;

		submesh.LODs.push_back(lod);
	}

	void Mesh::ReleaseCPUData() {
		for (Submesh& submesh : m_Submeshes) {
			submesh.Vertices.clear();
			submesh.Indices.clear();
			submesh.Meshlets.clear();
			submesh.MeshletVertices.clear();
			submesh.MeshletTriangles.clear();
			submesh.MeshletIndices.clear();

			submesh.Vertices.shrink_to_fit();
			submesh.Indices.shrink_to_fit();
			submesh.Meshlets.shrink_to_fit();
			submesh.MeshletVertices.shrink_to_fit();
			submesh.MeshletTriangles.shrink_to_fit();
			submesh.MeshletIndices.shrink_to_fit();
		}
	}

	void Mesh::Destroy() {
		for (Submesh& submesh : m_Submeshes)
			Renderer::GetMaterialManager()->RTDestroyMaterial(submesh.MaterialID);
	}
}