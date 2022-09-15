#include "lypch.h"

#include "Shader.h"
#include "VulkanGraphicsShader.h"
#include "VulkanComputeShader.h"

#include "Core/FileSystem.h"

#include "shaderc/shaderc.hpp"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<Shader> Shader::Create(const std::string& name, const std::string& path) {
		Ref<Shader> instance = nullptr;
		auto extension = FileSystem::GetFileExtension(path);

		if (extension == ".comp") {
			if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
				instance = Memory::CreateRef<VulkanComputeShader>(name, path);

			return instance;
		} else if (extension == ".glsl") {
			if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
				instance = Memory::CreateRef<VulkanGraphicsShader>(name, path);

			return instance;
		}

		LUCY_CRITICAL("Shader extension not supported!");
		LUCY_ASSERT(false);
		return instance;
	}

	Shader::Shader(const std::string& name, const std::string& path)
		: m_Path(path), m_Name(name) {
	}

	std::vector<uint32_t> Shader::LoadSPIRVData(const std::string& path, shaderc::Compiler& compiler, shaderc::CompileOptions& options,
							   shaderc_shader_kind kind, const std::string& cachedData) {
		using Iter = std::vector<std::string>::iterator;

		auto LoadData = [](std::vector<std::string>& lines, Iter& from, Iter& to) {
			std::string buffer;
			if (lines.end() != from) {
				for (auto i = from; i != to; i++) {
					buffer += *i + "\n";
				}
			}

			return buffer;
		};

		std::vector<std::string> lines;
		FileSystem::ReadFileLine<std::string>(path, lines);

		std::string shaderType = "";
		Iter from, to;
		switch (kind) {
			case shaderc_shader_kind::shaderc_vertex_shader:
				shaderType = "Vertex";
				from = std::find(lines.begin(), lines.end(), "//type vertex");
				to = std::find(lines.begin(), lines.end(), "//type fragment");
				break;
			case shaderc_shader_kind::shaderc_fragment_shader:
				shaderType = "Fragment";
				from = std::find(lines.begin(), lines.end(), "//type fragment");
				to = lines.end();
				break;
			case shaderc_shader_kind::shaderc_compute_shader:
				shaderType = "Compute";
				from = lines.begin();
				to = lines.end();
				break;
		}

		std::string data = LoadData(lines, from, to);

		shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(data, kind, FileSystem::GetFileName(path).c_str(), options);
		uint32_t status = result.GetCompilationStatus();
		if (status != shaderc_compilation_status_success) {
			LUCY_CRITICAL(fmt::format("{0} Shader; Status: {1}, Message: {2}", shaderType, status, result.GetErrorMessage()));
			LUCY_ASSERT(false);
		}
		std::vector<uint32_t> dataAsSPIRV(result.cbegin(), result.cend());
		FileSystem::WriteToFile<uint32_t>(cachedData, dataAsSPIRV, OpenMode::Binary);

		return dataAsSPIRV;
	}

	std::vector<uint32_t> Shader::LoadSPIRVDataFromCache(const std::string& cachedFile) {
		std::vector<uint32_t> data;
		FileSystem::ReadFile<uint32_t>(cachedFile, data, OpenMode::Binary);
		return data;
	}
}