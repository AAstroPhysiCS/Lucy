#include "lypch.h"
#include "GraphicsShader.h"

#include "shaderc/shaderc.hpp"

#include "Core/Application.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	GraphicsShader::GraphicsShader(const std::string& name, const std::string& path)
		: Shader(name, path) {
	}

	void GraphicsShader::Load() {
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_performance);
		options.SetGenerateDebugInfo();

		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

		auto& filesystem = Application::Get()->GetFilesystem();

		const auto [vertexFileExtension, fragmentFileExtension] = GetCachedFileExtension();
		const std::string& cacheFileVert = filesystem.GetParentPath(m_Path) + "/" + filesystem.GetFileName(m_Path) + vertexFileExtension;
		const std::string& cacheFileFrag = filesystem.GetParentPath(m_Path) + "/" + filesystem.GetFileName(m_Path) + fragmentFileExtension;

		if (!filesystem.FileExists(cacheFileVert) || !filesystem.FileExists(cacheFileFrag)) {
			std::vector<uint32_t> dataVert = LoadSPIRVData(m_Path, compiler, options, shaderc_shader_kind::shaderc_vertex_shader, cacheFileVert);
			std::vector<uint32_t> dataFrag = LoadSPIRVData(m_Path, compiler, options, shaderc_shader_kind::shaderc_fragment_shader, cacheFileFrag);

			m_Reflect.Info(m_Path, dataVert, VK_SHADER_STAGE_VERTEX_BIT);
			m_Reflect.Info(m_Path, dataFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
			LoadInternal(dataVert, dataFrag);
		} else {
			std::vector<uint32_t> dataVert = LoadSPIRVDataFromCache(cacheFileVert);
			std::vector<uint32_t> dataFrag = LoadSPIRVDataFromCache(cacheFileFrag);

			m_Reflect.Info(m_Path, dataVert, VK_SHADER_STAGE_VERTEX_BIT);
			m_Reflect.Info(m_Path, dataFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
			LoadInternal(dataVert, dataFrag);
		}
	}

	const GraphicsShader::Extensions GraphicsShader::GetCachedFileExtension() {
		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			return Extensions{ ".cached_vulkan.vert", ".cached_vulkan.frag" };
		}
		return Extensions{ ".undefined", ".undefined" };
	}
}