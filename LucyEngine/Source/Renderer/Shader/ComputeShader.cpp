#include "lypch.h"
#include "ComputeShader.h"
#include "VulkanComputeShader.h"

#include "Core/Application.h"

#include "Renderer/Renderer.h"

namespace Lucy {
	
	ComputeShader::ComputeShader(const std::string& name, const std::string& path)
		: Shader(name, path) {
	}

	void ComputeShader::Load() {
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_performance);
		options.SetGenerateDebugInfo();

		auto& filesystem = Application::Get()->GetFilesystem();

		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

		const char* computeFileExtension = ".cached_vulkan.comp";
		const std::string& cacheCompute = filesystem.GetParentPath(m_Path) + "/" + filesystem.GetFileName(m_Path) + computeFileExtension;

		std::vector<uint32_t> dataCompute;

		if (!filesystem.FileExists(cacheCompute))
			dataCompute = LoadSPIRVData(m_Path, compiler, options, shaderc_shader_kind::shaderc_compute_shader, cacheCompute);
		else
			dataCompute = LoadSPIRVDataFromCache(cacheCompute);

		m_Reflect.Info(m_Path, dataCompute, VK_SHADER_STAGE_COMPUTE_BIT);
		LoadInternal(dataCompute);
	}
}