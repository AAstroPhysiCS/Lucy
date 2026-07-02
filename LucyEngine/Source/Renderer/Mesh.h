#pragma once

#include <array>

#include "vulkan/vulkan.h"

#include "assimp/scene.h"

#include "Material/Material.h"

#include "Device/RenderResource.h"

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
			return decltype(Position)::length()
				+ decltype(MeshID)::length()
				+ decltype(TexCoords)::length()
				+ decltype(Normal)::length()
				+ decltype(Tangent)::length()
				+ decltype(Bitangent)::length();
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

	struct Submesh {
		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;

		MaterialID MaterialID = -1;

		glm::mat4 Transform = glm::mat4{1.0f};

		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;
		uint32_t BaseVertexCount = 0;
		uint32_t BaseIndexCount = 0;
	};

	struct MetadataInfo {
		uint32_t TotalIndicesSize = 0;
		uint32_t TotalVerticesSize = 0;
	};

	class Mesh : public MemoryTrackable {
	public:
		static Ref<Mesh> Create(const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
		static Ref<Mesh> Create(const std::string& path);

		Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		Mesh(const std::string& path);
		~Mesh() = default;

		inline std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }

		inline std::string& GetName() { return m_Name; }
		inline const glm::vec3& GetMeshID() const { return m_MeshID; }
		inline std::string& GetPath() { return m_Path; }

		inline RenderResourceHandle GetVertexBufferHandle() { return m_VertexBufferHandle; }
		inline RenderResourceHandle GetIndexBufferHandle() { return m_IndexBufferHandle; }

		inline MetadataInfo GetMetadataInfo() const { return m_MetadataInfo; }

		void Destroy();
	private:
		inline static std::atomic_uint32_t s_NextMeshID = 1;

		void Load(const Ref<RenderDevice>& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		void Load();

		void LoadProgram(const aiScene* scene);
		void TraverseHierarchy(const aiNode* node, const glm::mat4& parentTransform);

		RenderResourceHandle m_VertexBufferHandle = InvalidRenderResourceHandle;
		RenderResourceHandle m_IndexBufferHandle = InvalidRenderResourceHandle;

		std::vector<Submesh> m_Submeshes;
		std::string m_Path;
		std::string m_Name;

		glm::vec3 m_MeshID = glm::vec3(-1.0f);
		MetadataInfo m_MetadataInfo;
	private:
		friend glm::vec3 AllocateMeshID();
	};
}

