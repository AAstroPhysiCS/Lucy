#include "lypch.h"

#include "OpenGLMaterial.h"
#include "Renderer/Image/OpenGLImage.h"
#include "Renderer/Context/Pipeline.h"

#include "Renderer/Memory/Buffer/OpenGL/OpenGLUniformBuffer.h"
#include "Renderer/Shader/OpenGLShader.h"

namespace Lucy {

	OpenGLMaterial::OpenGLMaterial(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath)
		: Material(shader, aiMaterial, submeshName, importedFilePath) {
		LoadTexture(aiMaterial, 1, ALBEDO_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 2, NORMALS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 3, METALLIC_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 4, ROUGHNESS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 5, AO_TYPE, importedFilePath);
	}

	void OpenGLMaterial::Update(Ref<Pipeline> pipeline) {
	}

	void OpenGLMaterial::Bind(Ref<Pipeline> pipeline) {
		m_Shader.As<OpenGLShader>()->Bind();
		for (uint32_t i = 0; i < m_Textures.size(); i++) {
			Ref<OpenGLImage2D> texture = m_Textures[i].As<OpenGLImage2D>();
			uint32_t slot = texture->GetSlot();
			glBindTextureUnit(slot, texture->GetID());
		}

		auto& uniformBuffer = pipeline->GetUniformBuffers<OpenGLUniformBuffer>("TextureSlots");

		//TODO: Kinda bad/ugly code and its slow somehow although i dont do anything wrong here...!
		if (HasTexture(Material::ALBEDO_TYPE)) {
			int32_t albedoSlot = GetTexture(Material::ALBEDO_TYPE).As<OpenGLImage2D>()->GetSlot();
			uniformBuffer->SetData((void*)&albedoSlot, sizeof(int32_t), 0);
		} else if (HasTexture(Material::NORMALS_TYPE)) {
			int32_t normalsSlot = GetTexture(Material::NORMALS_TYPE).As<OpenGLImage2D>()->GetSlot();
			uniformBuffer->SetData((void*)&normalsSlot, sizeof(int32_t), sizeof(int32_t));
		} else if (HasTexture(Material::METALLIC_TYPE)) {
			int32_t metallicSlot = GetTexture(Material::METALLIC_TYPE).As<OpenGLImage2D>()->GetSlot();
			uniformBuffer->SetData((void*)&metallicSlot, sizeof(int32_t), sizeof(int32_t) * 2);
		} else if (HasTexture(Material::ROUGHNESS_TYPE)) {
			int32_t roughnessSlot = GetTexture(Material::ROUGHNESS_TYPE).As<OpenGLImage2D>()->GetSlot();
			uniformBuffer->SetData((void*)&roughnessSlot, sizeof(int32_t), sizeof(int32_t) * 3);
		} else if (HasTexture(Material::AO_TYPE)) {
			int32_t aoSlot = GetTexture(Material::AO_TYPE).As<OpenGLImage2D>()->GetSlot();
			uniformBuffer->SetData((void*)&aoSlot, sizeof(int32_t), sizeof(int32_t) * 4);
		}
	}

	void OpenGLMaterial::Unbind(Ref<Pipeline> pipeline) {
		int32_t clearValue = -1;
		auto& uniformBuffer = pipeline->GetUniformBuffers<OpenGLUniformBuffer>("TextureSlots");
		uniformBuffer->SetData((void*)&clearValue, sizeof(int32_t) * 5, 0);
		m_Shader.As<OpenGLShader>()->Unbind();
		for (uint32_t i = 0; i < m_Textures.size(); i++) {
			Ref<OpenGLImage2D> texture = m_Textures[i].As<OpenGLImage2D>();
			glBindTextureUnit(texture->GetSlot(), 0);
		}
	}

	void OpenGLMaterial::LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath) {
		aiString path;
		if (aiMaterial->GetTexture((aiTextureType)type.Type, 0, &path) == aiReturn_SUCCESS) {
			std::string properTexturePath = FileSystem::GetParentPath(importedFilePath) + "/" + std::string(path.data);

			ImageCreateInfo createInfo;
			createInfo.GenerateMipmap = true;
			createInfo.Parameter = { 0, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR };

			Ref<OpenGLRHIImageDesc> imageDesc = Memory::CreateRef<OpenGLRHIImageDesc>();
			imageDesc->PixelType = OpenGLRHIImageDesc::PixelType::UnsignedByte;
			imageDesc->Slot = slot;
			createInfo.InternalInfo = imageDesc;

			Ref<Image2D> texture2D = Image2D::Create(properTexturePath, createInfo);
			m_Textures.push_back(texture2D);
		} else {
			LUCY_WARN(fmt::format("Texture {0} could not be loaded: {1}", type.Name, path.data));
		}
	}
}