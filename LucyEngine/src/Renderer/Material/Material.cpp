#include "lypch.h"

#include "Material.h"
#include "OpenGLMaterial.h"
#include "VulkanMaterial.h"

#include "Renderer/Renderer.h"

namespace Lucy {
	
	Ref<Material> Material::Create(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLMaterial>(shader, aiMaterial, submeshName, importedFilePath);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanMaterial>(shader, aiMaterial, submeshName, importedFilePath);
				break;
		}
		return nullptr;
	}

	Material::Material(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath)
		: m_Shader(shader) {
		aiColor3D diffuse;
		float shininess, reflectivity, roughness;

		aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
		aiMaterial->Get(AI_MATKEY_REFLECTIVITY, reflectivity);

		roughness = 1.0f - glm::sqrt(shininess / 100.0f);

		m_MaterialData = MaterialData(*(glm::vec3*)&diffuse, std::string(submeshName), shininess, reflectivity, roughness);
	}

	void Material::Destroy() {
		for (Ref<Image2D> image : m_Textures) {
			image->Destroy();
		}
	}
}