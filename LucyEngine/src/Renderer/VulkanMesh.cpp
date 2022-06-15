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
		m_VertexBuffer->LoadToGPU();
		m_IndexBuffer->LoadToGPU();
		m_Loaded = true;
	}
}