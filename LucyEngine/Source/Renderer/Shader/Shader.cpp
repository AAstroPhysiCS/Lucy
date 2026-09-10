#include "lypch.h"
#include "Shader.h"

namespace Lucy {
	
	Shader::Shader(const std::string& name, const std::filesystem::path& path, const std::string& entryPointName)
		: m_Path(path), m_Name(name), m_EntryPointName(entryPointName) {
	}

	void Shader::RunReflect(const Slang::ComPtr<IComponentType>& linkedProgram, ShaderStageType shaderStage, std::string_view entryPointName) {
		m_Reflect.Info(m_Path, linkedProgram, shaderStage, entryPointName);
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