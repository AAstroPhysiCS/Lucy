#pragma once

#include "Material.h"

namespace Lucy {

	class OpenGLMaterial : public Material {
	public:
		OpenGLMaterial(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, const std::string& importedFilePath);
		virtual ~OpenGLMaterial() = default;

		void Update(Ref<Pipeline> pipeline) override;
		void Bind(Ref<Pipeline> pipeline);
		void Unbind(Ref<Pipeline> pipeline);
	private:
		void LoadTexture(aiMaterial* aiMaterial, MaterialImageType type, const std::string& importedFilePath) override;
	};
}