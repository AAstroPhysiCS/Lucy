#include "lypch.h"
#include "VulkanMesh.h"

namespace Lucy {

	VulkanMesh::VulkanMesh(const std::string& path)
		: Mesh(path) {
	}

	void VulkanMesh::Bind() {
		if (!m_Loaded)
			LoadBuffers();
	}

	void VulkanMesh::Unbind() {
		
	}
	
	void VulkanMesh::LoadBuffers() {
		m_VertexBuffer->AddData(
			{ -0.5f, -0.5f,
			   0.5f, -0.5f,
			   0.5f, 0.5f,
			  -0.5f, 0.5f
			}
		);
		m_VertexBuffer->Load();

		m_IndexBuffer->AddData(
			{ 0, 1, 2, 2, 3, 0 }
		);
		m_IndexBuffer->Load();

		m_Loaded = true;
	}
}