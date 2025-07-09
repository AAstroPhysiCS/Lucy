#include "lypch.h"
#include "ComputeShader.h"

#include "Core/FileSystem.h"

#include "Renderer/Renderer.h"

namespace Lucy {
	
	ComputeShader::ComputeShader(const std::string& name, const std::filesystem::path& path, Ref<RenderDevice> device)
		: Shader(name, path) {
	}

	void ComputeShader::RTLoad(const Ref<RenderDevice>& device, bool forceReloadFromDisk) {
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetIncluder(std::make_unique<CustomShaderIncluder>(device));

		options.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_performance);
		options.SetGenerateDebugInfo();

		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

		const char* computeFileExtension = ".cached_vulkan.comp";

		const auto& cachedFolder = Shader::GetCacheFolder();
		auto cachedFolderWithName = cachedFolder / (std::filesystem::path(GetName()));

		bool success = FileSystem::CreateDir(cachedFolder);
		LUCY_ASSERT(success || FileSystem::DirectoryExists(cachedFolder),
			"Failed to create cache directory");

		std::filesystem::path cacheComputeFilePath = cachedFolderWithName.replace_extension(computeFileExtension);

		std::vector<uint32_t> dataCompute;

		if (!FileSystem::FileExists(cacheComputeFilePath) || forceReloadFromDisk) {
			auto path = GetPath();
			dataCompute = LoadSPIRVData(path, compiler, options, shaderc_shader_kind::shaderc_compute_shader);
			if (!forceReloadFromDisk)
				FileSystem::WriteToFile<uint32_t>(cacheComputeFilePath, dataCompute, OpenMode::Binary);
		} else {
			dataCompute = LoadSPIRVDataFromCache(cacheComputeFilePath);
		}

		RunReflect(dataCompute, VK_SHADER_STAGE_COMPUTE_BIT);
		LoadInternal(device, dataCompute);
	}
}