#include "lypch.h"
#include "VulkanComputeShader.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanContextDevice.h"

namespace Lucy {

	VulkanComputeShader::VulkanComputeShader(const std::string& name, const std::string& path)
		: ComputeShader(name, path) {
		Renderer::EnqueueToRenderThread([=]() {
			Load();
		});
	}

	void VulkanComputeShader::LoadInternal(const std::vector<uint32_t>& dataCompute) {
		VkShaderModuleCreateInfo computeCreateInfo{};
		computeCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		computeCreateInfo.codeSize = dataCompute.size() * sizeof(uint32_t);
		computeCreateInfo.pCode = dataCompute.data();

		const VulkanContextDevice& device = VulkanContextDevice::Get();
		LUCY_VK_ASSERT(vkCreateShaderModule(device.GetLogicalDevice(), &computeCreateInfo, nullptr, &m_ComputeShaderModule));

		m_ShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		m_ShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		m_ShaderStageInfo.module = m_ComputeShaderModule;
		m_ShaderStageInfo.pName = "main";
	}

	void VulkanComputeShader::Destroy() {
		const VulkanContextDevice& device = VulkanContextDevice::Get();
		vkDestroyShaderModule(device.GetLogicalDevice(), m_ComputeShaderModule, nullptr);
	}
}