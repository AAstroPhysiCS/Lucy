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
		m_VertexBuffer->Load();
		m_IndexBuffer->Load();
		m_Loaded = true;
	}
}