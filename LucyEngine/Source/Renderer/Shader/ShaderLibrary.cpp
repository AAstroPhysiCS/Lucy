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

		PushShader(Shader::Create("LucyIrradiance", "Assets/Shaders/LucyIrradiance.comp"));
		PushShader(Shader::Create("LucyComputeTest", "Assets/Shaders/LucyComputeTest.comp"));
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