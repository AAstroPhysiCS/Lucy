#include "lypch.h"
#include "ShaderLibrary.h"

#include "Shader.h"

#include "Core/FileSystem.h"

namespace Lucy {

	ShaderLibrary& ShaderLibrary::Get() {
		static ShaderLibrary s_Instance;
		return s_Instance;
	}

	void ShaderLibrary::Init() {
		PushShader(Shader::Create("LucyPBR", "Assets/Shaders/LucyPBR.glsl"));
		PushShader(Shader::Create("LucyID", "Assets/Shaders/LucyID.glsl"));
		PushShader(Shader::Create("LucyHDRSkybox", "Assets/Shaders/LucyHDRSkybox.glsl"));
		PushShader(Shader::Create("LucyImageToHDRConverter", "Assets/Shaders/LucyImageToHDRConverter.glsl"));

#if USE_COMPUTE_FOR_CUBEMAP_GEN
		PushShader(Shader::Create("LucyIrradianceGen", "Assets/Shaders/LucyIrradianceGen.comp"));
		PushShader(Shader::Create("LucyPrefilterGen", "Assets/Shaders/LucyPrefilterGen.comp"));
#else
		PushShader(Shader::Create("LucyIrradianceGen", "Assets/Shaders/LucyIrradianceGen.glsl"));
		PushShader(Shader::Create("LucyPrefilterGen", "Assets/Shaders/LucyPrefilterGen.glsl"));
#endif
	}

	void ShaderLibrary::Destroy() {
		for (const auto& shader : m_Shaders)
			shader->Destroy();
	}

	Ref<Shader> ShaderLibrary::GetShader(const std::string& name) const {
		for (auto& shader : m_Shaders) {
			if (shader->GetName() == name)
				return shader;
		}

		LUCY_CRITICAL("Shader not found!");
		LUCY_ASSERT(false);
		return nullptr;
	}

	void ShaderLibrary::PushShader(const Ref<Shader>& instance) {
		m_Shaders.push_back(instance);
	}
}