#include "lypch.h"

#include "Shader.h"
#include "VulkanGraphicsShader.h"
#include "VulkanComputeShader.h"
#include "VulkanUniformImageSampler.h"

#include "Core/FileSystem.h"

#include "shaderc/shaderc.hpp"

#include "Renderer/Renderer.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"
#include "Renderer/Image/VulkanImage.h"
#include "Renderer/Memory/Buffer/PushConstant.h"

namespace Lucy {
	
	Ref<Shader> Shader::Create(const std::string& name, const std::filesystem::path& path, Ref<RenderDevice> device) {
		Ref<Shader> resource = nullptr;

		if (auto extension = path.extension(); extension == ".comp") {
			if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
				resource = Memory::CreateRef<VulkanComputeShader>(name, path, device);
		} else if (extension == ".glsl") {
			if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
				resource = Memory::CreateRef<VulkanGraphicsShader>(name, path, device);
		}

		return resource;
	}

	Shader::Shader(const std::string& name, const std::filesystem::path& path)
		: m_Path(path), m_Name(name) {
	}

	VulkanPushConstant& Shader::GetPushConstants(const std::string& name) {
		for (VulkanPushConstant& pushConstant : m_PushConstants) {
			if (name == pushConstant.GetName()) {
				return pushConstant;
			}
		}
		LUCY_ASSERT(false, "Could not find a suitable Push Constant for the given name: {0}", name);
	}

	uint32_t Shader::BindImageHandleTo(const std::string& imageBufferName, const Ref<Image>& image) {
		LUCY_ASSERT(image != nullptr, "Binding image failed, because image '{0}' is nullptr!", imageBufferName);

		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			const Ref<VulkanImage> vulkanImage = image->As<VulkanImage>();

			for (const auto& handle : m_DescriptorSetHandles) {
				auto descriptorSet = GetDescriptorSetFromHandle(handle)->As<VulkanDescriptorSet>();

				if (auto imageSampler = descriptorSet->GetVulkanImageSampler(imageBufferName)) {
					imageSampler->ImageInfos.push_back(VulkanAPI::DescriptorImageInfo(
						vulkanImage->GetCurrentLayout(), vulkanImage->GetImageView().GetVulkanHandle(), vulkanImage->GetImageView().GetSampler()));
					return (uint32_t)imageSampler->ImageInfos.size() - 1;
				}
			}
			LUCY_ASSERT(false, "Could not find a image sampler with the name {0}", imageBufferName);
		}
		return 0;
	}

	bool Shader::HasImageHandleBoundTo(const std::string& imageBufferName) const {
		for (const auto& handle : m_DescriptorSetHandles) {
			auto descriptorSet = GetDescriptorSetFromHandle(handle)->As<VulkanDescriptorSet>();

			if (auto imageSampler = descriptorSet->GetVulkanImageSampler(imageBufferName))
				return !imageSampler->ImageInfos.empty();
		}
		LUCY_ASSERT(false, "Could not find a image sampler with the name {0}", imageBufferName);
		return false;
	}

	void Shader::RTLoadDescriptors(const Ref<RenderDevice>& device, const Ref<VulkanDescriptorPool>& descriptorPool) {
		const auto& reflectPushConstants = m_Reflect.GetShaderPushConstants();
		const auto& reflectUniformBlockMaps = m_Reflect.GetShaderUniformBlockMap();

		for (const auto& [set, info] : reflectUniformBlockMaps) {
			DescriptorSetCreateInfo createInfo{
				.SetIndex = set,
				.ShaderUniformBlocks = info,
			};
			RenderResourceHandle descriptorSetHandle = device->CreateDescriptorSet(createInfo);
			const auto& descriptorSet = device->AccessResource<VulkanDescriptorSet>(descriptorSetHandle);
			descriptorSet->RTBake(descriptorPool);
			m_DescriptorSetHandles.push_back(descriptorSetHandle); //maybe just store the handle?
		}

		for (auto& pc : reflectPushConstants)
			m_PushConstants.emplace_back(pc.Name, pc.BufferSize, 0, pc.StageFlag);
	}

	void Shader::RunReflect(const std::vector<uint32_t>& data, int32_t flags) {
		m_Reflect.Info(m_Path, data, flags);
	}

	void Shader::RTDestroyResource(const Ref<RenderDevice>& device) {
		m_PushConstants.clear();
		m_Reflect.DestroyCachedData();

		for (auto handles : m_DescriptorSetHandles)
			device->RTDestroyResource(handles);
		m_DescriptorSetHandles.clear();
	}

	std::vector<uint32_t> Shader::LoadSPIRVData(const std::filesystem::path& path, shaderc::Compiler& compiler, shaderc::CompileOptions& options,
							   shaderc_shader_kind kind) {
		using Iter = std::vector<std::string>::iterator;

		auto LoadData = [](std::vector<std::string>& lines, const Iter& from, const Iter& to) {
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
		LUCY_ASSERT(status == shaderc_compilation_status_success, "{0} Shader; Status: {1}, Message: {2}", shaderType, status, result.GetErrorMessage());
		std::vector<uint32_t> dataAsSPIRV(result.cbegin(), result.cend());

		return dataAsSPIRV;
	}

	std::vector<uint32_t> Shader::LoadSPIRVDataFromCache(const std::filesystem::path& cachedFilePath) {
		std::vector<uint32_t> data;
		FileSystem::ReadFile<uint32_t>(cachedFilePath, data, OpenMode::Binary);
		return data;
	}
}