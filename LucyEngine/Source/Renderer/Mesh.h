#pragma once

#include "assimp/scene.h"

#include "Material/Material.h"

#include "Device/RenderResource.h"

namespace Lucy {

	class RenderDevice;

	struct Submesh {
		std::vector<glm::vec3> Vertices;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec3> Tangents;
		std::vector<glm::vec3> BiTangents;
		std::vector<glm::vec2> TextureCoords;

		std::vector<uint32_t> Faces;
		MaterialID MaterialID;

		glm::mat4 Transform = glm::mat4(1.0f);

		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;
		uint32_t BaseVertexCount = 0;
		uint32_t BaseIndexCount = 0;
	};

	struct MetadataInfo {
		uint32_t TotalIndicesSize = 0;
		uint32_t TotalVerticesSize = 0;
	};

	static int32_t MESH_ID_COUNT_X = 0;
	static int32_t MESH_ID_COUNT_Y = 0;
	static int32_t MESH_ID_COUNT_Z = 0;
	static int32_t MESH_ID_COUNT_W = 0;

	class Mesh : public MemoryTrackable {
	public:
		static Ref<Mesh> Create(const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
		static Ref<Mesh> Create(const std::string& path);

		Mesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
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
		void Load(Ref<RenderDevice>& device, const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
		void Load();

		void LoadData(const aiScene* scene);
		void TraverseHierarchy(const aiNode* node, const glm::mat4& parentTransform);

		RenderResourceHandle m_VertexBufferHandle = InvalidRenderResourceHandle;
		RenderResourceHandle m_IndexBufferHandle = InvalidRenderResourceHandle;

		std::vector<Submesh> m_Submeshes;
		std::string m_Path;
		std::string m_Name;

		glm::vec3 m_MeshID = glm::vec3(-1.0f);
		MetadataInfo m_MetadataInfo;
	private:
		friend void IncreaseMeshCount(Mesh* m);
	};
}

