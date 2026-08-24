#pragma once

#include <array>

#include "vulkan/vulkan.h"

#include "assimp/scene.h"
#include "assimp/Importer.hpp"

#include "Material/Material.h"

#include "Device/RenderDeviceResource.h"

namespace Lucy {

	class RenderDevice;

	struct Vertex final {
		glm::vec3 Position = glm::vec3{0.0f};
		glm::vec3 MeshID = glm::vec3{0.0f};
		glm::vec2 TexCoords = glm::vec2{0.0f};
		glm::vec3 Normal = glm::vec3{0.0f};
		glm::vec3 Tangent = glm::vec3{0.0f};
		glm::vec3 Bitangent = glm::vec3{0.0f};

		[[nodiscard]] static consteval uint32_t GetComponentCount() {
			return decltype(Position)::length() + decltype(MeshID)::length() + decltype(TexCoords)::length() 
				+ decltype(Normal)::length() + decltype(Tangent)::length() + decltype(Bitangent)::length();
		}

		[[nodiscard]] static constexpr VkVertexInputBindingDescription GetBindingDescription() {
			return {
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			};
		}

		[[nodiscard]] static constexpr std::array<VkVertexInputAttributeDescription, 6> GetAttributeDescriptions(uint32_t binding) {
			return {
				VkVertexInputAttributeDescription{
					.location = 0,
					.binding = binding,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, Position)
				},
				VkVertexInputAttributeDescription{
					.location = 1,
					.binding = binding,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, MeshID)
				},
				VkVertexInputAttributeDescription{
					.location = 2,
					.binding = binding,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Vertex, TexCoords)
				},
				VkVertexInputAttributeDescription{
					.location = 3,
					.binding = binding,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, Normal)
				},
				VkVertexInputAttributeDescription{
					.location = 4,
					.binding = binding,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, Tangent)
				},
				VkVertexInputAttributeDescription{
					.location = 5,
					.binding = binding,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, Bitangent)
				}
			};
		}
	};

	struct Meshlet {
		uint32_t VertexCount = 0;
		uint32_t TriangleCount = 0;

		uint32_t FirstIndex = 0;
		uint32_t IndexCount = 0;

		glm::vec4 BoundingSphere = glm::vec4{ 0.0f };
		glm::vec4 NormalCone = glm::vec4{ 0.0f };
		glm::vec3 AABBCenter{};
		glm::vec3 AABBExtents{};
	};

	struct SubmeshLOD {
		uint32_t FirstMeshlet = 0;
		uint32_t MeshletCount = 0;

		uint32_t FirstMeshletIndex = 0;
		uint32_t MeshletIndexCount = 0;

		float Error = 0.0f;
	};

	struct Submesh {
		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;

		std::vector<Meshlet> Meshlets;

		std::vector<SubmeshLOD> LODs;

		//Flattened uint32_t index stream used by ordinary indexed rendering.
		std::vector<uint32_t> MeshletIndices;

		RenderDeviceObjectHandle MaterialID{};

		glm::mat4 Transform = glm::mat4{ 1.0f };

		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;
		uint32_t MeshletCount = 0;

		uint32_t BaseVertexCount = 0;
		uint32_t BaseIndexCount = 0;

		uint32_t BaseMeshletCount = 0;
		uint32_t BaseMeshletIndexCount = 0;
	};

	struct MetadataInfo {
		uint32_t TotalIndicesSize = 0;
		uint32_t TotalVerticesSize = 0;
		uint32_t TotalMeshletsSize = 0;
		uint32_t TotalMeshletVerticesSize = 0;
		uint32_t TotalMeshletTrianglesSize = 0;
		uint32_t TotalMeshletIndicesSize = 0;
	};

	class Mesh : public MemoryTrackable {
	public:
		template <size_t NVert, size_t NInd>
		Mesh(const std::array<float, NVert>& vertices, const std::array<uint32_t, NInd>& indices)
			: Mesh(ConvertVerticesFromFloatToVertex(vertices), std::vector<uint32_t>(indices.begin(), indices.end())) {
		}
		Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
		Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices);
		Mesh(const std::string& path);
		~Mesh() = default;

		Mesh(const Mesh& other) = delete;
		Mesh(Mesh&& other) noexcept = delete;
		Mesh& operator=(const Mesh& other) = delete;
		Mesh& operator=(Mesh&& other) noexcept = delete;

		std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }

		std::string& GetName() { return m_Name; }
		const glm::vec3& GetMeshID() const { return m_MeshID; }
		std::string& GetPath() { return m_Path; }

		MetadataInfo GetMetadataInfo() const { return m_MetadataInfo; }
		uint32_t GetIndicesSize() const { return m_MetadataInfo.TotalIndicesSize; }

		const RenderDeviceObjectHandle& GetRenderDeviceMeshHandle() const { return m_RenderDeviceMeshHandle; }

		void Destroy();
	private:
		static inline std::atomic_uint32_t s_NextMeshID = 1;

		template <size_t N>
		[[nodiscard]] constexpr static std::vector<Vertex> ConvertVerticesFromFloatToVertex(const std::array<float, N>& vertices) {
			LUCY_ASSERT(vertices.size() % 3 == 0, "Position array must contain complete vec3 values.");

			size_t vertexCount = vertices.size() / 3;
			std::vector<Vertex> convertedVertices(vertexCount);

			for (size_t i = 0; i < vertexCount; i++) {
				size_t sourceIndex = i * 3;
				convertedVertices[i].Position = { vertices[sourceIndex + 0], vertices[sourceIndex + 1], vertices[sourceIndex + 2] };
			}

			return convertedVertices;
		}

		void Load(const Ref<RenderDevice>& device, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
		void Load();

		void LoadProgram(const aiScene* scene);
		void TraverseHierarchy(const aiNode* node, const glm::mat4& parentTransform);
	private:
		constexpr static inline float MESHOPT_OVERDRAW_THRESHOLD = 1.05f;
		constexpr static inline uint32_t MESH_LOD_COUNT = 4;
		constexpr static float MIN_LOD_REDUCTION = 0.90f;

		constexpr static inline std::array<float, MESH_LOD_COUNT> MESH_LOD_RATIOS = {
			1.00f,
			0.70f,
			0.40f,
			0.20f
		};

		constexpr static inline float MESH_LOD_TARGET_ERROR = 0.01f;

		constexpr static inline size_t MESHLET_MIN_TRIANGLES = 4;
		constexpr static inline size_t MESHLET_MAX_VERTICES = 64;
		constexpr static inline size_t MESHLET_MAX_TRIANGLES = 64;
		constexpr static inline float MESHLET_CONE_WEIGHT = 0.5f;
	private:
		void ReleaseCPUData();

		void OptimizeMeshData(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
		void BuildLODs(Submesh& submesh);

		void BuildMeshlets(Submesh& submesh, const std::vector<uint32_t>& indices, float lodError, uint32_t lodIndex);

		RenderDeviceObjectHandle m_RenderDeviceMeshHandle{};

		std::vector<Submesh> m_Submeshes;

		std::string m_Path;
		std::string m_Name;

		glm::vec3 m_MeshID = glm::vec3(-1.0f);
		MetadataInfo m_MetadataInfo;

		Unique<Assimp::Importer> m_Importer = nullptr;
	private:
		friend glm::vec3 AllocateMeshID();
	};
}

