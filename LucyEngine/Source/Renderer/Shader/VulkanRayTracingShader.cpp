#include "lypch.h"
#include "VulkanRayTracingShader.h"

#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanRayTracingShader::VulkanRayTracingShader(const std::string& name, const std::filesystem::path& path, const std::string& entryPointName,
		ShaderStageType stage, Ref<RenderDevice> device, const std::span<const uint32_t>& data)
		: Shader(name, path, entryPointName), m_Stage(stage) {
		RTLoad(device, { data });
	}

	void VulkanRayTracingShader::RTLoad(const Ref<RenderDevice>& device, const std::vector<std::span<const uint32_t>>& datas) {
		LUCY_ASSERT(datas.size() == 1);
		LoadInternal(device, datas[0]);
	}

	void VulkanRayTracingShader::LoadInternal(const Ref<RenderDevice>& device, const std::span<const uint32_t>& data) {
		VkShaderModuleCreateInfo shaderCreateInfo = VulkanAPI::ShaderModuleCreateInfo(data.size() * sizeof(uint32_t), data.data());

		VkDevice logicalDevice = device->As<VulkanRenderDevice>()->GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreateShaderModule(logicalDevice, &shaderCreateInfo, nullptr, &m_ShaderModule));
		m_ShaderStageInfo = VulkanAPI::PipelineShaderStageCreateInfo(ShaderStageToVkShaderStage(m_Stage), m_ShaderModule, GetEntryPointName().c_str());
	}

	void VulkanRayTracingShader::RTDestroyResource(const Ref<RenderDevice>& device) {
		Shader::RTDestroyResource(device);

		if (!m_ShaderModule)
			return;

		VkDevice logicalDevice = device->As<VulkanRenderDevice>()->GetLogicalDevice();
		vkDestroyShaderModule(logicalDevice, m_ShaderModule, nullptr);

		m_ShaderModule = VK_NULL_HANDLE;
	}
}