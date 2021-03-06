#pragma once

#include "Material.h"

namespace Lucy {

	class VulkanMaterial : public Material {
	public:
		VulkanMaterial(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, const std::string& importedFilePath);
		virtual ~VulkanMaterial() = default;

		void Update(Ref<Pipeline> pipeline) override;
	private:
		void LoadTexture(aiMaterial* aiMaterial, MaterialImageType type, const std::string& importedFilePath) override;
	};
}