#include "lypch.h"

#include "Shader.h"
#include "VulkanGraphicsShader.h"
#include "VulkanComputeShader.h"

#include "Core/Application.h"

#include "shaderc/shaderc.hpp"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<Shader> Shader::Create(const std::string& name, const std::string& path) {
		Ref<Shader> instance = nullptr;
		auto extension = Application::Get()->GetFileSystem().GetFileExtension(path);

		if (extension == ".comp") {
			if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
				instance = Memory::CreateRef<VulkanComputeShader>(name, path);

			return instance;
		} else if (extension == ".glsl") {
			if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
				instance = Memory::CreateRef<VulkanGraphicsShader>(name, path);

			return instance;
		}

		LUCY_ASSERT(false, "Shader extension not supported!");
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

		auto& filesystem = Application::Get()->GetFileSystem();

		std::vector<std::string> lines;
		filesystem.ReadFileLine<std::string>(path, lines);

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

		shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(data, kind, filesystem.GetFileName(path).c_str(), options);
		uint32_t status = result.GetCompilationStatus();
		LUCY_ASSERT(status == shaderc_compilation_status_success, "{0} Shader; Status: {1}, Message: {2}", shaderType, status, result.GetErrorMessage());
		std::vector<uint32_t> dataAsSPIRV(result.cbegin(), result.cend());
		filesystem.WriteToFile<uint32_t>(cachedData, dataAsSPIRV, OpenMode::Binary);

		return dataAsSPIRV;
	}

	std::vector<uint32_t> Shader::LoadSPIRVDataFromCache(const std::string& cachedFile) {
		std::vector<uint32_t> data;
		Application::Get()->GetFileSystem().ReadFile<uint32_t>(cachedFile, data, OpenMode::Binary);
		return data;
	}
}