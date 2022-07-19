#include "lypch.h"

#include "Shader.h"
#include "OpenGLShader.h"
#include "VulkanShader.h"

#include "../Renderer.h"
#include "Utils/Utils.h"
#include "Core/FileSystem.h"

namespace Lucy {

	Shader::Shader(const std::string& name, const std::string& path)
		: m_Path(path), m_Name(name) {
		Renderer::Enqueue([=]() {
			Load();
		});
	}

	Ref<Shader> Shader::Create(const std::string& name, const std::string& path) {
		auto currentRenderArchitecture = Renderer::GetCurrentRenderArchitecture();
		Ref<Shader> instance = nullptr;
		if (currentRenderArchitecture == RenderArchitecture::OpenGL) {
			instance = Memory::CreateRef<OpenGLShader>(name, path);
		} else if (currentRenderArchitecture == RenderArchitecture::Vulkan) {
			instance = Memory::CreateRef<VulkanShader>(name, path);
		}
		return instance;
	}

	Ref<Shader> ShaderLibrary::GetShader(const std::string& name) {
		for (auto& shader : m_Shaders) {
			if (shader->GetName() == name)
				return shader;
		}

		LUCY_CRITICAL("Shader not found!");
		LUCY_ASSERT(false);
	}

	void ShaderLibrary::PushShader(const Ref<Shader>& instance) {
		m_Shaders.push_back(instance);
	}

	void Shader::Load() {
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_performance);
		options.SetGenerateDebugInfo();

		if (Renderer::GetCurrentRenderArchitecture() == RenderArchitecture::OpenGL) {
			options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
		} else if (Renderer::GetCurrentRenderArchitecture() == RenderArchitecture::Vulkan) {
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		}

		const auto [vertexFileExtension, fragmentFileExtension] = GetCachedFileExtension();
		const std::string& cacheFileVert = FileSystem::GetParentPath(m_Path) + "/" + FileSystem::GetFileName(m_Path) + vertexFileExtension;
		const std::string& cacheFileFrag = FileSystem::GetParentPath(m_Path) + "/" + FileSystem::GetFileName(m_Path) + fragmentFileExtension;

		if (!FileSystem::FileExists(cacheFileVert) || !FileSystem::FileExists(cacheFileFrag))
			LoadAndRedoCache(compiler, options, cacheFileVert, cacheFileFrag);
		else
			LoadFromCache(cacheFileVert, cacheFileFrag);
	}

	void Shader::LoadAndRedoCache(shaderc::Compiler& compiler, shaderc::CompileOptions& options, const std::string& cacheFileVert, const std::string& cacheFileFrag) {
		std::vector<std::string> lines;
		FileSystem::ReadFileLine<std::string>(m_Path, lines);

		std::string& vertex = LoadVertexData(lines);
		std::string& fragment = LoadFragmentData(lines);

		shaderc::SpvCompilationResult resultVertex = compiler.CompileGlslToSpv(vertex, shaderc_shader_kind::shaderc_glsl_vertex_shader, FileSystem::GetFileName(m_Path).c_str(), options);
		uint32_t status = resultVertex.GetCompilationStatus();
		if (status != shaderc_compilation_status_success) {
			LUCY_CRITICAL(fmt::format("Vertex Shader; Status: {0}, Message: {1}", status, resultVertex.GetErrorMessage()));
			LUCY_ASSERT(false);
		}
		std::vector<uint32_t> dataVert(resultVertex.cbegin(), resultVertex.cend());
		FileSystem::WriteToFile<uint32_t>(cacheFileVert, dataVert, OpenMode::Binary);

		shaderc::SpvCompilationResult resultFrag = compiler.CompileGlslToSpv(fragment, shaderc_shader_kind::shaderc_glsl_fragment_shader, FileSystem::GetFileName(m_Path).c_str(), options);
		status = resultFrag.GetCompilationStatus();
		if (status != shaderc_compilation_status_success) {
			LUCY_CRITICAL(fmt::format("Fragment Shader; Status: {0}, Message: {1}", status, resultFrag.GetErrorMessage()));
			LUCY_ASSERT(false);
		}

		std::vector<uint32_t> dataFrag(resultFrag.cbegin(), resultFrag.cend());
		FileSystem::WriteToFile<uint32_t>(cacheFileFrag, dataFrag, OpenMode::Binary);

		m_Reflect.Info(m_Path, dataVert, dataFrag);
		LoadInternal(dataVert, dataFrag);
	}

	void Shader::LoadFromCache(const std::string& cacheFileVert, const std::string& cacheFileFrag) {
		std::vector<uint32_t> dataVert;
		std::vector<uint32_t> dataFrag;

		FileSystem::ReadFile<uint32_t>(cacheFileVert, dataVert, OpenMode::Binary);
		FileSystem::ReadFile<uint32_t>(cacheFileFrag, dataFrag, OpenMode::Binary);

		m_Reflect.Info(m_Path, dataVert, dataFrag);
		LoadInternal(dataVert, dataFrag);
	}

	std::string Shader::LoadVertexData(std::vector<std::string>& lines) {
		const auto& from = std::find(lines.begin(), lines.end(), "//type vertex");
		const auto& to = std::find(lines.begin(), lines.end(), "//type fragment");

		std::string buffer;
		if (lines.end() != from) {
			for (auto i = from; i != to; i++) {
				buffer += *i + "\n";
			}
		}

		return buffer;
	}

	std::string Shader::LoadFragmentData(std::vector<std::string>& lines) {
		const auto& from = std::find(lines.begin(), lines.end(), "//type fragment");
		const auto& to = lines.end();

		std::string buffer;
		if (lines.end() != from) {
			for (auto i = from; i != to; i++) {
				buffer += *i + "\n";
			}
		}

		return buffer;
	}

	const Shader::Extensions Shader::GetCachedFileExtension() {
		if (Renderer::GetCurrentRenderArchitecture() == RenderArchitecture::OpenGL) {
			return Extensions{ ".cached_opengl.vert", ".cached_opengl.frag" };
		} else {
			return Extensions{ ".cached_vulkan.vert", ".cached_vulkan.frag" };
		}
	}
}