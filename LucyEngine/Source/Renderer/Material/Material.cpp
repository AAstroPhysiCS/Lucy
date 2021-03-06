#include "lypch.h"

#include "Material.h"
#include "OpenGLMaterial.h"
#include "VulkanMaterial.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<Material> Material::Create(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, const std::string& importedFilePath) {
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

	Material::Material(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, const std::string& importedFilePath)
		: m_Shader(shader) {
		aiColor3D diffuse;
		float shininess, reflectivity, roughness;

		aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
		aiMaterial->Get(AI_MATKEY_REFLECTIVITY, reflectivity);

		if (reflectivity < 0)
			reflectivity = 0.0f;
		
		if (shininess < 0)
			shininess = 0.0f;

		roughness = 1.0f - glm::sqrt(shininess / 100.0f);

		m_MaterialData = MaterialData(*(glm::vec3*)&diffuse, std::string(submeshName), shininess, reflectivity, roughness);
	}

	void Material::SetImage(MaterialImageType type, Ref<Image2D> image) {
		m_Textures.insert(m_Textures.begin() + type.Index, image);
	}

	void Material::Destroy() {
		for (Ref<Image2D> image : m_Textures) {
			image->Destroy();
		}
		s_MaterialIDProvider.ReturnID(m_MaterialID);
	}
}