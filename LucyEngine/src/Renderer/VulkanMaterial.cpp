#include "lypch.h"
#include "VulkanMaterial.h"
#include "Context/Pipeline.h"

namespace Lucy {

	VulkanMaterial::VulkanMaterial(RefLucy<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath)
		: Material(shader, aiMaterial, submeshName, importedFilePath) {
	}

	void VulkanMaterial::Bind(RefLucy<Pipeline> pipeline) {

	}

	void VulkanMaterial::Unbind(RefLucy<Pipeline> pipeline) {

	}

	void VulkanMaterial::LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath) {

	}
}