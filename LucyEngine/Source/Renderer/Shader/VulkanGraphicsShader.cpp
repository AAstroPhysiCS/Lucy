#include "lypch.h"
#include "VulkanGraphicsShader.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanGraphicsShader::VulkanGraphicsShader(const std::string& name, const std::filesystem::path& path, Ref<RenderDevice> device)
		: GraphicsShader(name, path) {
		RTLoad(device);
	}

	void VulkanGraphicsShader::LoadInternal(const Ref<RenderDevice>& device, const std::vector<uint32_t>& dataVertex, const std::vector<uint32_t>& dataFragment) {
		VkShaderModuleCreateInfo vertexCreateInfo = VulkanAPI::ShaderModuleCreateInfo(dataVertex.size() * sizeof(uint32_t), dataVertex.data());
		VkShaderModuleCreateInfo fragmentCreateInfo = VulkanAPI::ShaderModuleCreateInfo(dataFragment.size() * sizeof(uint32_t), dataFragment.data());

		const auto& vulkanDevice = device->As<VulkanRenderDevice>();
		LUCY_VK_ASSERT(vkCreateShaderModule(vulkanDevice->GetLogicalDevice(), &vertexCreateInfo, nullptr, &m_VertexShaderModule));
		LUCY_VK_ASSERT(vkCreateShaderModule(vulkanDevice->GetLogicalDevice(), &fragmentCreateInfo, nullptr, &m_FragmentShaderModule));

		m_ShaderStageInfos[0] = VulkanAPI::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShaderModule, "main");
		m_ShaderStageInfos[1] = VulkanAPI::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShaderModule, "main");
	}

	void VulkanGraphicsShader::RTDestroyResource(const Ref<RenderDevice>& device) {
		Shader::RTDestroyResource(device);
		if (!m_VertexShaderModule || !m_FragmentShaderModule) return;

		const auto& vulkanDevice = device->As<VulkanRenderDevice>();
		vkDestroyShaderModule(vulkanDevice->GetLogicalDevice(), m_VertexShaderModule, nullptr);
		vkDestroyShaderModule(vulkanDevice->GetLogicalDevice(), m_FragmentShaderModule, nullptr);
	}
}