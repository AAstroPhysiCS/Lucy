#include "lypch.h"

#include "OpenGLMaterial.h"
#include "Image/OpenGLImage.h"
#include "Context/Pipeline.h"

namespace Lucy {

	OpenGLMaterial::OpenGLMaterial(RefLucy<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath)
		: Material(shader, aiMaterial, submeshName, importedFilePath) {
		LoadTexture(aiMaterial, 1, ALBEDO_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 2, NORMALS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 3, METALLIC_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 4, ROUGHNESS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 5, AO_TYPE, importedFilePath);
	}

	void OpenGLMaterial::Bind(RefLucy<Pipeline> pipeline) {
		m_Shader->Bind();
		for (uint32_t i = 0; i < m_Textures.size(); i++) {
			RefLucy<OpenGLImage2D> texture = As(m_Textures[i], OpenGLImage2D);
			uint32_t slot = texture->GetSlot();
			glBindTextureUnit(slot, texture->GetID());
		}

		auto& uniformBuffer = pipeline->GetUniformBuffers<UniformBuffer>(0);

		//TODO: Kinda bad/ugly code and its slow somehow although i dont do anything wrong here...!
		if (HasTexture(Material::ALBEDO_TYPE)) {
			int32_t albedoSlot = As(GetTexture(Material::ALBEDO_TYPE), OpenGLImage2D)->GetSlot();
			uniformBuffer->SetData((void*)&albedoSlot, sizeof(int32_t), 0);
		} else if (HasTexture(Material::NORMALS_TYPE)) {
			int32_t normalsSlot = As(GetTexture(Material::NORMALS_TYPE), OpenGLImage2D)->GetSlot();
			uniformBuffer->SetData((void*)&normalsSlot, sizeof(int32_t), sizeof(int32_t));
		} else if (HasTexture(Material::METALLIC_TYPE)) {
			int32_t metallicSlot = As(GetTexture(Material::METALLIC_TYPE), OpenGLImage2D)->GetSlot();
			uniformBuffer->SetData((void*)&metallicSlot, sizeof(int32_t), sizeof(int32_t) * 2);
		} else if (HasTexture(Material::ROUGHNESS_TYPE)) {
			int32_t roughnessSlot = As(GetTexture(Material::ROUGHNESS_TYPE), OpenGLImage2D)->GetSlot();
			uniformBuffer->SetData((void*)&roughnessSlot, sizeof(int32_t), sizeof(int32_t) * 3);
		} else if (HasTexture(Material::AO_TYPE)) {
			int32_t aoSlot = As(GetTexture(Material::AO_TYPE), OpenGLImage2D)->GetSlot();
			uniformBuffer->SetData((void*)&aoSlot, sizeof(int32_t), sizeof(int32_t) * 4);
		}
	}

	void OpenGLMaterial::Unbind(RefLucy<Pipeline> pipeline) {
		int32_t clearValue = -1;
		auto& uniformBuffer = pipeline->GetUniformBuffers<UniformBuffer>(0);
		uniformBuffer->SetData((void*)&clearValue, sizeof(int32_t) * 5, 0);
		m_Shader->Unbind();
		for (uint32_t i = 0; i < m_Textures.size(); i++) {
			RefLucy<OpenGLImage2D> texture = As(m_Textures[i], OpenGLImage2D);
			glBindTextureUnit(texture->GetSlot(), 0);
		}
	}

	void OpenGLMaterial::LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath) {
		aiString path;
		if (aiMaterial->GetTexture((aiTextureType)type.Type, 0, &path) == aiReturn_SUCCESS) {
			std::string properTexturePath = FileSystem::GetParentPath(importedFilePath) + "/" + std::string(path.data);

			ImageSpecification specs;
			specs.GenerateMipmap = true;
			specs.Parameter = { 0, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR };

			RefLucy<OpenGLRHIImageDesc> imageDesc = CreateRef<OpenGLRHIImageDesc>();
			imageDesc->PixelType = OpenGLRHIImageDesc::PixelType::UnsignedByte;
			imageDesc->Slot = slot;
			specs.InternalInfo = imageDesc;

			RefLucy<Image2D> texture2D = Image2D::Create(properTexturePath, specs);
			m_Textures.push_back(texture2D);
		} else {
			LUCY_WARN(fmt::format("Texture {0} could not be loaded: {1}", type.Name, path.data));
		}
	}
}