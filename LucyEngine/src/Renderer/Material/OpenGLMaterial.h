#pragma once

#include "Material.h"

namespace Lucy {

	class OpenGLMaterial : public Material {
	public:
		OpenGLMaterial(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath);
		virtual ~OpenGLMaterial() = default;

		void Update(Ref<Pipeline> pipeline) override;
		void Bind(Ref<Pipeline> pipeline);
		void Unbind(Ref<Pipeline> pipeline);
	private:
		void LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath) override;
	};
}