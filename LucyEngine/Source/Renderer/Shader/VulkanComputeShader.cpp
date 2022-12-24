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
		VkShaderModuleCreateInfo computeCreateInfo = VulkanAPI::ShaderModuleCreateInfo(dataCompute.size() * sizeof(uint32_t), dataCompute.data());

		const VulkanContextDevice& device = VulkanContextDevice::Get();
		LUCY_VK_ASSERT(vkCreateShaderModule(device.GetLogicalDevice(), &computeCreateInfo, nullptr, &m_ComputeShaderModule));
		m_ShaderStageInfo = VulkanAPI::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, m_ComputeShaderModule, "main");
	}

	void VulkanComputeShader::Destroy() {
		const VulkanContextDevice& device = VulkanContextDevice::Get();
		vkDestroyShaderModule(device.GetLogicalDevice(), m_ComputeShaderModule, nullptr);
	}
}