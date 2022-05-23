#include "lypch.h"

#include "Material.h"
#include "Context/Pipeline.h"

#include "Image/Image.h"

#include "OpenGLMaterial.h"
#include "VulkanMaterial.h"

#include "Renderer.h"

namespace Lucy {
	
	RefLucy<Material> Material::Create(RefLucy<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLMaterial>(shader, aiMaterial, submeshName, importedFilePath);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanMaterial>(shader, aiMaterial, submeshName, importedFilePath);
				break;
		}
		return nullptr;
	}

	Material::Material(RefLucy<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath)
		: m_Shader(shader) {
		aiColor3D diffuse;
		float shininess, reflectivity, roughness;

		aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
		aiMaterial->Get(AI_MATKEY_REFLECTIVITY, reflectivity);

		roughness = 1.0f - glm::sqrt(shininess / 100.0f);

		m_MaterialData = MaterialData(*(glm::vec3*)&diffuse, std::string(submeshName), shininess, reflectivity, roughness);
	}
}