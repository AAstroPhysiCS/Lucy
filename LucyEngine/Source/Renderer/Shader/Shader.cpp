#include "lypch.h"
#include "Shader.h"

namespace Lucy {
	
	Shader::Shader(const std::string& name, const std::filesystem::path& path)
		: m_Path(path), m_Name(name) {
	}

	void Shader::RunReflect(const Slang::ComPtr<slang::IComponentType>& program, ShaderStageType stageFlag) {
		m_Reflect.Info(GetPath(), program, stageFlag);
	}

	void Shader::RTDestroyResource(const Ref<RenderDevice>& device) {
		m_Reflect.DestroyCachedData();
	}

	void Shader::PrintReflectInfo() {
		const auto& stageInfo = GetShaderInfo();
		LUCY_INFO(std::format("{0} constant/uniform buffers", stageInfo.ConstantBufferCount));
		LUCY_INFO(std::format("{0} sampled images", stageInfo.SampledImagesCount));
		LUCY_INFO(std::format("{0} storage images", stageInfo.StorageImageCount));
		LUCY_INFO(std::format("{0} storage buffers", stageInfo.StorageBufferCount));
	}
}