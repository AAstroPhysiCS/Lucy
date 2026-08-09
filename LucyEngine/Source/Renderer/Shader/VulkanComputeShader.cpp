#include "lypch.h"
#include "VulkanComputeShader.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanComputeShader::VulkanComputeShader(const std::string& name, const std::filesystem::path& path, const std::string& entryPointName, 
		Ref<RenderDevice> device, const std::span<const uint32_t>& dataCompute) 
		: ComputeShader(name, path, entryPointName) {
		RTLoad(device, { dataCompute });
	}

	void VulkanComputeShader::LoadInternal(const Ref<RenderDevice>& device, const std::span<const uint32_t>& dataCompute) {
		VkShaderModuleCreateInfo computeCreateInfo = VulkanAPI::ShaderModuleCreateInfo(dataCompute.size() * sizeof(uint32_t), dataCompute.data());

		VkDevice logicalDevice = device->As<VulkanRenderDevice>()->GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreateShaderModule(logicalDevice, &computeCreateInfo, nullptr, &m_ComputeShaderModule));
		m_ShaderStageInfo = VulkanAPI::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, m_ComputeShaderModule, GetEntryPointName().c_str());
	}

	void VulkanComputeShader::RTDestroyResource(const Ref<RenderDevice>& device) {
		Shader::RTDestroyResource(device);
		if (!m_ComputeShaderModule) return;

		VkDevice logicalDevice = device->As<VulkanRenderDevice>()->GetLogicalDevice();
		vkDestroyShaderModule(logicalDevice, m_ComputeShaderModule, nullptr);
	}
}