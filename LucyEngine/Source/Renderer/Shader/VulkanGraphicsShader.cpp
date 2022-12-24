#include "lypch.h"
#include "VulkanGraphicsShader.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanGraphicsShader::VulkanGraphicsShader(const std::string& path, const std::string& name)
		: GraphicsShader(path, name) {
		Renderer::EnqueueToRenderThread([=]() {
			Load();
		});
	}

	void VulkanGraphicsShader::LoadInternal(const std::vector<uint32_t>& dataVertex, const std::vector<uint32_t>& dataFragment) {
		VkShaderModuleCreateInfo vertexCreateInfo = VulkanAPI::ShaderModuleCreateInfo(dataVertex.size() * sizeof(uint32_t), dataVertex.data());
		VkShaderModuleCreateInfo fragmentCreateInfo = VulkanAPI::ShaderModuleCreateInfo(dataFragment.size() * sizeof(uint32_t), dataFragment.data());

		const VulkanContextDevice& device = VulkanContextDevice::Get();
		LUCY_VK_ASSERT(vkCreateShaderModule(device.GetLogicalDevice(), &vertexCreateInfo, nullptr, &m_VertexShaderModule));
		LUCY_VK_ASSERT(vkCreateShaderModule(device.GetLogicalDevice(), &fragmentCreateInfo, nullptr, &m_FragmentShaderModule));

		m_ShaderStageInfos[0] = VulkanAPI::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShaderModule, "main");
		m_ShaderStageInfos[1] = VulkanAPI::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShaderModule, "main");
	}

	void VulkanGraphicsShader::Destroy() {
		if (!m_VertexShaderModule || !m_FragmentShaderModule) return;
		const VulkanContextDevice& device = VulkanContextDevice::Get();
		vkDestroyShaderModule(device.GetLogicalDevice(), m_VertexShaderModule, nullptr);
		vkDestroyShaderModule(device.GetLogicalDevice(), m_FragmentShaderModule, nullptr);
	}
}