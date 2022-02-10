#include "lypch.h"
#include "Shader.h"
#include "OpenGLShader.h"
#include "VulkanShader.h"

#include "../Renderer.h"
#include "Utils.h"
#include "Core/FileSystem.h"

#include "shaderc/shaderc.hpp"

#include "spirv_cross/spirv_cross.hpp"
#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_reflect.hpp"

namespace Lucy {

	Shader::Shader(const std::string& path, const std::string& name)
		: m_Path(path), m_Name(name) {
		Renderer::Submit([=]() {
			Load();
		});
	}

	RefLucy<Shader> Shader::Create(const std::string& name, const std::string& path) {
		auto currentRenderArchitecture = Renderer::GetCurrentRenderArchitecture();
		RefLucy<Shader> instance = nullptr;
		if (currentRenderArchitecture == RenderArchitecture::OpenGL) {
			instance = CreateRef<OpenGLShader>(path, name);
		} else if (currentRenderArchitecture == RenderArchitecture::Vulkan) {
			instance = CreateRef<VulkanShader>(path, name);
		}
		Renderer::GetShaderLibrary().PushShader(instance);
		return instance;
	}

	RefLucy<Shader> ShaderLibrary::GetShader(const std::string& name) {
		for (auto& shader : m_Shaders) {
			if (shader->GetName() == name) return shader;
		}

		LUCY_CRITICAL("Shader not found!");
		LUCY_ASSERT(false);
	}

	void ShaderLibrary::PushShader(const RefLucy<Shader>& instance) {
		m_Shaders.push_back(instance);
	}

	void Shader::Load() {
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_performance);

		if (Renderer::GetCurrentRenderArchitecture() == RenderArchitecture::OpenGL) {
			options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
		} else if (Renderer::GetCurrentRenderArchitecture() == RenderArchitecture::Vulkan) {
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		}

		const auto [vertexFileExtension, fragmentFileExtension] = GetCachedFileExtension();
		std::string& cacheFileVert = FileSystem::GetParentPath(m_Path) + "/" + FileSystem::GetFileName(m_Path) + vertexFileExtension;
		std::string& cacheFileFrag = FileSystem::GetParentPath(m_Path) + "/" + FileSystem::GetFileName(m_Path) + fragmentFileExtension;

		if (!FileSystem::FileExists(cacheFileVert) || !FileSystem::FileExists(cacheFileFrag))
			LoadAndRedoCache(compiler, options, cacheFileVert, cacheFileFrag);
		else
			LoadFromCache(cacheFileVert, cacheFileFrag);
	}

	void Shader::LoadAndRedoCache(shaderc::Compiler& compiler, shaderc::CompileOptions& options, const std::string& cacheFileVert, const std::string& cacheFileFrag) {
		std::vector<std::string>& lines = Utils::ReadFile(m_Path);
		std::string& vertex = LoadVertexData(lines);
		std::string& fragment = LoadFragmentData(lines);

		shaderc::SpvCompilationResult resultVertex = compiler.CompileGlslToSpv(vertex, shaderc_shader_kind::shaderc_glsl_vertex_shader, FileSystem::GetFileName(m_Path).c_str(), options);
		if (resultVertex.GetCompilationStatus() != shaderc_compilation_status_success) {
			LUCY_CRITICAL(resultVertex.GetErrorMessage());
			LUCY_ASSERT(false);
		}
		std::vector<uint32_t> dataVert(resultVertex.cbegin(), resultVertex.cend());

		std::ofstream ofVert(cacheFileVert, std::ios::binary | std::ios::out);
		if (ofVert.is_open()) {
			ofVert.write((const char*)dataVert.data(), dataVert.size() * sizeof(uint32_t));
			ofVert.flush();
			ofVert.close();
		}

		shaderc::SpvCompilationResult resultFrag = compiler.CompileGlslToSpv(fragment, shaderc_shader_kind::shaderc_glsl_fragment_shader, FileSystem::GetFileName(m_Path).c_str(), options);
		if (resultFrag.GetCompilationStatus() != shaderc_compilation_status_success) {
			LUCY_CRITICAL(resultVertex.GetErrorMessage());
			LUCY_ASSERT(false);
		}

		std::vector<uint32_t> dataFrag(resultFrag.cbegin(), resultFrag.cend());

		std::ofstream ofFrag(cacheFileFrag, std::ios::binary | std::ios::out);
		if (ofFrag.is_open()) {
			ofFrag.write((const char*)dataFrag.data(), dataFrag.size() * sizeof(uint32_t));
			ofFrag.flush();
			ofFrag.close();
		}

		Info(dataVert, dataFrag);
		LoadInternal(dataVert, dataFrag);
	}

	void Shader::LoadFromCache(const std::string& cacheFileVert, const std::string& cacheFileFrag) {
		std::vector<uint32_t> dataVert;
		std::vector<uint32_t> dataFrag;

		std::ifstream isVert(cacheFileVert, std::ios::in | std::ios::binary);
		if (isVert.is_open()) {
			isVert.seekg(0, std::ios::end);
			size_t size = isVert.tellg();
			isVert.seekg(0, std::ios::beg);

			dataVert.resize(size / sizeof(uint32_t));
			isVert.read((char*)dataVert.data(), size);
			isVert.close();
		}

		std::ifstream isFrag(cacheFileFrag, std::ios::in | std::ios::binary);
		if (isFrag.is_open()) {
			isFrag.seekg(0, std::ios::end);
			size_t size = isFrag.tellg();
			isFrag.seekg(0, std::ios::beg);

			dataFrag.resize(size / sizeof(uint32_t));
			isFrag.read((char*)dataFrag.data(), size);
			isFrag.close();
		}

		Info(dataVert, dataFrag);
		LoadInternal(dataVert, dataFrag);
	}

	void Shader::Info(std::vector<uint32_t>& dataVertex, std::vector<uint32_t>& dataFragment) {
		//heap allocating it because of the compiler warning/stack size: "function uses X bytes of stack consider moving some data to heap"
		spirv_cross::Compiler* compilerVertex = new spirv_cross::Compiler(dataVertex);
		spirv_cross::Compiler* compilerFragment = new spirv_cross::Compiler(dataFragment);

		spirv_cross::ShaderResources resourcesVertex = compilerVertex->get_shader_resources();
		spirv_cross::ShaderResources resourcesFragment = compilerFragment->get_shader_resources();

		m_ShaderInfoVertex.UniformCount = resourcesVertex.uniform_buffers.size();
		m_ShaderInfoVertex.SampledImagesCount = resourcesVertex.sampled_images.size();
		m_ShaderInfoVertex.PushConstantBufferCount = resourcesVertex.push_constant_buffers.size();
		m_ShaderInfoVertex.StageInputCount = resourcesVertex.stage_inputs.size();
		m_ShaderInfoVertex.StageOutputCount = resourcesVertex.stage_outputs.size();

		LUCY_INFO(fmt::format("------Vertex Shader {0}------", m_Path));
		LUCY_INFO(fmt::format("{0} uniform buffers", m_ShaderInfoVertex.UniformCount));
		LUCY_INFO(fmt::format("{0} sampled images", m_ShaderInfoVertex.SampledImagesCount));
		LUCY_INFO(fmt::format("{0} push constant buffers", m_ShaderInfoVertex.PushConstantBufferCount));
		LUCY_INFO(fmt::format("{0} stage inputs", m_ShaderInfoVertex.StageInputCount));
		LUCY_INFO(fmt::format("{0} stage outputs", m_ShaderInfoVertex.StageOutputCount));

		for (const auto& ub : resourcesVertex.uniform_buffers) {
			const auto& bufferType = compilerVertex->get_type(ub.base_type_id);
			uint32_t set = compilerVertex->get_decoration(ub.id, spv::DecorationDescriptorSet);
			uint32_t bufferSize = compilerVertex->get_declared_struct_size(bufferType);
			uint32_t binding = compilerVertex->get_decoration(ub.id, spv::DecorationBinding);
			int32_t memberCount = bufferType.member_types.size();

			LUCY_INFO(fmt::format("{0}", ub.name));
			LUCY_INFO(fmt::format("Size = {0}", bufferSize));
			LUCY_INFO(fmt::format("Binding = {0}", binding));
			LUCY_INFO(fmt::format("Set = {0}", set));
			LUCY_INFO(fmt::format("Members = {0}", memberCount));

			ShaderUniformBufferInfo info;
			info.Set = set;
			info.Binding = binding;
			info.BufferSize = bufferSize;
			info.MemberCount = memberCount;
			info.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			info.Name = ub.name.c_str();
			info.StageFlag = VK_SHADER_STAGE_VERTEX_BIT;

			if (!m_DescriptorSetMap.count(set)) {
				std::vector<ShaderUniformBufferInfo> buffer;
				buffer.push_back(info);
				m_DescriptorSetMap.emplace(set, buffer);
			} else {
				auto& it = m_DescriptorSetMap.find(set);
				it->second.push_back(info);
			}
		}

		for (const auto& ui : resourcesVertex.sampled_images) {
			const auto& bufferType = compilerVertex->get_type(ui.base_type_id);
			uint32_t set = compilerVertex->get_decoration(ui.id, spv::DecorationDescriptorSet);
			uint32_t binding = compilerVertex->get_decoration(ui.id, spv::DecorationBinding);
			int32_t memberCount = bufferType.member_types.size();

			LUCY_INFO(fmt::format("{0}", ui.name));
			LUCY_INFO(fmt::format("Set = {0}", set));
			LUCY_INFO(fmt::format("Binding = {0}", binding));
			LUCY_INFO(fmt::format("Members = {0}", memberCount));

			ShaderUniformBufferInfo info;
			info.Set = set;
			info.Binding = binding;
			info.BufferSize = 0;
			info.MemberCount = memberCount;
			info.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			info.Name = ui.name.c_str();
			info.StageFlag = VK_SHADER_STAGE_VERTEX_BIT;

			if (!m_DescriptorSetMap.count(set)) {
				std::vector<ShaderUniformBufferInfo> buffer;
				buffer.push_back(info);
				m_DescriptorSetMap.emplace(set, buffer);
			} else {
				auto& it = m_DescriptorSetMap.find(set);
				it->second.push_back(info);
			}
		}

		m_ShaderInfoFragment.UniformCount = resourcesFragment.uniform_buffers.size();
		m_ShaderInfoFragment.SampledImagesCount = resourcesFragment.sampled_images.size();
		m_ShaderInfoFragment.PushConstantBufferCount = resourcesFragment.push_constant_buffers.size();
		m_ShaderInfoFragment.StageInputCount = resourcesFragment.stage_inputs.size();
		m_ShaderInfoFragment.StageOutputCount = resourcesFragment.stage_outputs.size();

		LUCY_INFO(fmt::format("------Fragment Shader {0}------", m_Path));
		LUCY_INFO(fmt::format("{0} uniform buffers", m_ShaderInfoFragment.UniformCount));
		LUCY_INFO(fmt::format("{0} sampled images", m_ShaderInfoFragment.SampledImagesCount));
		LUCY_INFO(fmt::format("{0} push constant buffers", m_ShaderInfoFragment.PushConstantBufferCount));
		LUCY_INFO(fmt::format("{0} stage inputs", m_ShaderInfoFragment.StageInputCount));
		LUCY_INFO(fmt::format("{0} stage outputs", m_ShaderInfoFragment.StageOutputCount));

		for (const auto& ub : resourcesFragment.uniform_buffers) {
			const auto& bufferType = compilerFragment->get_type(ub.base_type_id);
			uint32_t set = compilerFragment->get_decoration(ub.id, spv::DecorationDescriptorSet);
			uint32_t bufferSize = compilerFragment->get_declared_struct_size(bufferType);
			uint32_t binding = compilerFragment->get_decoration(ub.id, spv::DecorationBinding);
			int32_t memberCount = bufferType.member_types.size();

			LUCY_INFO(fmt::format("{0}", ub.name));
			LUCY_INFO(fmt::format("Set = {0}", set));
			LUCY_INFO(fmt::format("Size = {0}", bufferSize));
			LUCY_INFO(fmt::format("Binding = {0}", binding));
			LUCY_INFO(fmt::format("Members = {0}", memberCount));

			ShaderUniformBufferInfo info;
			info.Set = set;
			info.Binding = binding;
			info.BufferSize = bufferSize;
			info.MemberCount = memberCount;
			info.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			info.Name = ub.name.c_str();
			info.StageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

			if (!m_DescriptorSetMap.count(set)) {
				std::vector<ShaderUniformBufferInfo> buffer;
				buffer.push_back(info);
				m_DescriptorSetMap.emplace(set, buffer);
			} else {
				auto& it = m_DescriptorSetMap.find(set);
				it->second.push_back(info);
			}
		}

		for (const auto& ui : resourcesFragment.sampled_images) {
			const auto& bufferType = compilerFragment->get_type(ui.base_type_id);
			uint32_t set = compilerFragment->get_decoration(ui.id, spv::DecorationDescriptorSet);
			uint32_t binding = compilerFragment->get_decoration(ui.id, spv::DecorationBinding);
			int32_t memberCount = bufferType.member_types.size();

			LUCY_INFO(fmt::format("{0}", ui.name));
			LUCY_INFO(fmt::format("Set = {0}", set));
			LUCY_INFO(fmt::format("Binding = {0}", binding));
			LUCY_INFO(fmt::format("Members = {0}", memberCount));

			ShaderUniformBufferInfo info;
			info.Set = set;
			info.Binding = binding;
			info.BufferSize = 0;
			info.MemberCount = memberCount;
			info.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			info.Name = ui.name.c_str();
			info.StageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

			if (!m_DescriptorSetMap.count(set)) {
				std::vector<ShaderUniformBufferInfo> buffer;
				buffer.push_back(info);
				m_DescriptorSetMap.emplace(set, buffer);
			} else {
				auto& it = m_DescriptorSetMap.find(set);
				it->second.push_back(info);
			}
		}

		delete compilerVertex;
		delete compilerFragment;
	}

	std::string Shader::LoadVertexData(std::vector<std::string>& lines) {
		auto& from = std::find(lines.begin(), lines.end(), "//type vertex");
		auto& to = std::find(lines.begin(), lines.end(), "//type fragment");

		std::string buffer;
		if (lines.end() != from) {
			for (auto i = from; i != to; i++) {
				buffer += *i + "\n";
			}
		}

		return buffer;
	}

	std::string Shader::LoadFragmentData(std::vector<std::string>& lines) {
		auto& from = std::find(lines.begin(), lines.end(), "//type fragment");
		auto& to = lines.end();

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