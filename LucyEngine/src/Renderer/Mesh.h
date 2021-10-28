#pragma once

#include "Buffer/OpenGL/OpenGLVertexBuffer.h"
#include "Buffer/OpenGL/OpenGLIndexBuffer.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "Material.h"

#include "glm/glm.hpp"

namespace Lucy {

	struct Submesh {
		std::vector<glm::vec3> Vertices;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec3> Tangents;
		std::vector<glm::vec3> BiTangents;
		std::vector<glm::vec2> TextureCoords;

		glm::mat4 Transform;
		uint32_t MaterialIndex;

		uint32_t VertexCount;
		uint32_t IndexCount;
		uint32_t BaseVertexCount;
		uint32_t BaseIndexCount;
	};

	class Mesh
	{
	public:
		Mesh(const std::string& path);
		Mesh(const Mesh& other) = default;
		
		static RefLucy<Mesh> Create(const std::string& path);

		inline std::string& GetPath() { return m_Path; }
	private:

		void LoadData(const aiScene* scene, uint32_t& totalSize);
		void LoadMaterials(const aiScene* scene);
		void TraverseHierarchy(const aiNode* node, const aiNode* rootNode);
		
		RefLucy<VertexBuffer> m_VertexBuffer;
		RefLucy<IndexBuffer> m_IndexBuffer;

		std::vector<Submesh> m_Submeshes;
		std::vector<Material> m_Materials;
		std::string m_Path;
	};
}

