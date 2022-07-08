#pragma once

#include "Material.h"
#include "Renderer/Memory/Buffer/UniformBuffer.h"

namespace Lucy {

	class Pipeline;

	class VulkanMaterial : public Material {
	public:
		VulkanMaterial(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath);
		virtual ~VulkanMaterial() = default;

		void Update(Ref<Pipeline> pipeline) override;
	private:
		void LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath) override;
	};
}