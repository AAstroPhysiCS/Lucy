#pragma once

#include "Material.h"
#include "Memory/Buffer/UniformBuffer.h"

namespace Lucy {

	class OpenGLMaterial : public Material {
	public:
		OpenGLMaterial(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath);
		virtual ~OpenGLMaterial() = default;

		void Bind(Ref<Pipeline> pipeline);
		void Unbind(Ref<Pipeline> pipeline);
	private:
		void LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath);
	};
}