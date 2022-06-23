#pragma once

#include "Memory/Buffer/VertexBuffer.h"
#include "Memory/Buffer/IndexBuffer.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "OpenGLMaterial.h"
#include "VulkanMaterial.h"

#include "glm/glm.hpp"

namespace Lucy {

	struct Submesh {
		std::vector<glm::vec3> Vertices;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec3> Tangents;
		std::vector<glm::vec3> BiTangents;
		std::vector<glm::vec2> TextureCoords;

		std::vector<uint32_t> Faces;

		glm::mat4 Transform = glm::mat4(1.0f);
		uint32_t MaterialIndex = 0;

		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;
		uint32_t BaseVertexCount = 0;
		uint32_t BaseIndexCount = 0;
	};

	static int MESH_ID_COUNT_X = 0;
	static int MESH_ID_COUNT_Y = 0;
	static int MESH_ID_COUNT_Z = 0;

	class Mesh {
	public:
		Mesh(const std::string& path);
		virtual ~Mesh();

		static Ref<Mesh> Create(const std::string& path);

		inline std::string& GetPath() { return m_Path; }
		inline std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }
		inline std::vector<Ref<Material>>& GetMaterials() { return m_Materials; }
		inline std::string& GetName() { return m_Name; }
		inline glm::vec3 GetMeshPixelValue() { return m_PixelValue; }

		inline Ref<VertexBuffer> GetVertexBuffer() { return m_VertexBuffer; }
		inline Ref<IndexBuffer> GetIndexBuffer() { return m_IndexBuffer; }

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
	protected:
		virtual void LoadBuffers() = 0;
		void LoadData(const aiScene* scene, uint32_t& totalSize);
		void LoadMaterials(const aiScene* scene, const aiMesh* mesh);
		void TraverseHierarchy(const aiNode* node, const aiNode* rootNode);

		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;

		std::vector<Submesh> m_Submeshes;
		std::vector<Ref<Material>> m_Materials;
		std::string m_Path;
		std::string m_Name;

		glm::vec3 m_PixelValue;

		bool m_Loaded = false;
	private:
		friend static void IncreaseMeshCount(Mesh* m);
	};
}

