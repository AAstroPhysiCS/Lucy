#include "lypch.h"
#include "GraphicsShader.h"

#include "shaderc/shaderc.hpp"

#include "Core/FileSystem.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	GraphicsShader::GraphicsShader(const std::string& name, const std::filesystem::path& path)
		: Shader(name, path) {
	}

	void GraphicsShader::RTLoad(const Ref<RenderDevice>& device, bool forceReloadFromDisk) {
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_zero);
		options.SetGenerateDebugInfo();

		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

		const auto& [vertexFileExtension, fragmentFileExtension] = GetCachedFileExtension();

		std::filesystem::path cacheFileVert = GetPath().replace_extension(vertexFileExtension);
		std::filesystem::path cacheFileFrag = GetPath().replace_extension(fragmentFileExtension);

		if (!FileSystem::FileExists(cacheFileVert) || !FileSystem::FileExists(cacheFileFrag) || forceReloadFromDisk) {
			auto path = GetPath();
			std::vector<uint32_t> dataVert = LoadSPIRVData(path, compiler, options, shaderc_shader_kind::shaderc_vertex_shader);
			std::vector<uint32_t> dataFrag = LoadSPIRVData(path, compiler, options, shaderc_shader_kind::shaderc_fragment_shader);
			
			if (!forceReloadFromDisk) {
				FileSystem::WriteToFile<uint32_t>(cacheFileVert, dataVert, OpenMode::Binary);
				FileSystem::WriteToFile<uint32_t>(cacheFileFrag, dataFrag, OpenMode::Binary);
			}

			RunReflect(dataVert, VK_SHADER_STAGE_VERTEX_BIT);
			RunReflect(dataFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
			LoadInternal(device, dataVert, dataFrag);
		} else {
			std::vector<uint32_t> dataVert = LoadSPIRVDataFromCache(cacheFileVert);
			std::vector<uint32_t> dataFrag = LoadSPIRVDataFromCache(cacheFileFrag);

			RunReflect(dataVert, VK_SHADER_STAGE_VERTEX_BIT);
			RunReflect(dataFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
			LoadInternal(device, dataVert, dataFrag);
		}
	}

	GraphicsShader::Extensions GraphicsShader::GetCachedFileExtension() const {
		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			return Extensions{ ".cached_vulkan.vert", ".cached_vulkan.frag" };
		}
		return Extensions{ ".undefined", ".undefined" };
	}
}