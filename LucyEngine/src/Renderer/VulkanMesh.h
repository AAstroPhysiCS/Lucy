#pragma once

#include "Mesh.h"

namespace Lucy {

	class VulkanMesh : public Mesh {
	public:
		VulkanMesh(const std::string& path);
		VulkanMesh(const VulkanMesh& other) = default;
		virtual ~VulkanMesh() = default;

		void Bind();
		void Unbind();
	private:
		void LoadBuffers();
	};
}