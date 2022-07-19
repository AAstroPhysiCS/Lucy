#include "lypch.h"

#include "OpenGLMaterial.h"
#include "Renderer/Image/OpenGLImage.h"
#include "Renderer/Context/Pipeline.h"

#include "Renderer/Memory/Buffer/OpenGL/OpenGLUniformBuffer.h"
#include "Renderer/Shader/OpenGLShader.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLMaterial::OpenGLMaterial(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, const std::string& importedFilePath)
		: Material(shader, aiMaterial, submeshName, importedFilePath) {
		LoadTexture(aiMaterial, ALBEDO_TYPE, importedFilePath);
		LoadTexture(aiMaterial, NORMALS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, METALLIC_TYPE, importedFilePath);
		LoadTexture(aiMaterial, ROUGHNESS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, AO_TYPE, importedFilePath);
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
		if (HasImage(Material::ALBEDO_TYPE)) {
			int32_t albedoSlot = GetImage(Material::ALBEDO_TYPE).As<OpenGLImage2D>()->GetSlot();
			uniformBuffer->SetData((void*)&albedoSlot, sizeof(int32_t), 0);
		} else if (HasImage(Material::NORMALS_TYPE)) {
			int32_t normalsSlot = GetImage(Material::NORMALS_TYPE).As<OpenGLImage2D>()->GetSlot();
			uniformBuffer->SetData((void*)&normalsSlot, sizeof(int32_t), sizeof(int32_t));
		} else if (HasImage(Material::METALLIC_TYPE)) {
			int32_t metallicSlot = GetImage(Material::METALLIC_TYPE).As<OpenGLImage2D>()->GetSlot();
			uniformBuffer->SetData((void*)&metallicSlot, sizeof(int32_t), sizeof(int32_t) * 2);
		} else if (HasImage(Material::ROUGHNESS_TYPE)) {
			int32_t roughnessSlot = GetImage(Material::ROUGHNESS_TYPE).As<OpenGLImage2D>()->GetSlot();
			uniformBuffer->SetData((void*)&roughnessSlot, sizeof(int32_t), sizeof(int32_t) * 3);
		} else if (HasImage(Material::AO_TYPE)) {
			int32_t aoSlot = GetImage(Material::AO_TYPE).As<OpenGLImage2D>()->GetSlot();
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

	void OpenGLMaterial::LoadTexture(aiMaterial* aiMaterial, MaterialImageType type, const std::string& importedFilePath) {
		aiString path;
		if (aiMaterial->GetTexture((aiTextureType)type.Type, 0, &path) == aiReturn_SUCCESS) {
			std::string properTexturePath = FileSystem::GetParentPath(importedFilePath) + "/" + std::string(path.data);

			ImageCreateInfo createInfo;
			createInfo.GenerateMipmap = true;
			createInfo.Parameter = { 0, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR };

			Ref<OpenGLRHIImageDesc> imageDesc = Memory::CreateRef<OpenGLRHIImageDesc>();
			imageDesc->PixelType = OpenGLRHIImageDesc::PixelType::UnsignedByte;
			createInfo.InternalInfo = imageDesc;

			Ref<Image2D> texture2D = Image2D::Create(properTexturePath, createInfo);
			m_Textures.push_back(texture2D);
		} else {
			LUCY_WARN(fmt::format("Texture {0} could not be loaded: {1}", type.Name, path.data));
		}
	}
}