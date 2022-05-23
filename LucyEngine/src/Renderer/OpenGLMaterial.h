#pragma once

#include "Material.h"
#include "Buffer/UniformBuffer.h"

namespace Lucy {

	class OpenGLMaterial : public Material {
	public:
		OpenGLMaterial(RefLucy<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath);
		virtual ~OpenGLMaterial() = default;

		void Bind(RefLucy<Pipeline> pipeline);
		void Unbind(RefLucy<Pipeline> pipeline);
	private:
		void LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath);
	};
}