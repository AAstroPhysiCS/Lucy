#pragma once

#include "Material.h"
#include "Buffer/UniformBuffer.h"

namespace Lucy {

	class Pipeline;

	class VulkanMaterial : public Material {
	public:
		VulkanMaterial(RefLucy<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath);
		virtual ~VulkanMaterial() = default;

		void Bind(RefLucy<Pipeline> pipeline);
		void Unbind(RefLucy<Pipeline> pipeline);
	private:
		void LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath);
	};
}