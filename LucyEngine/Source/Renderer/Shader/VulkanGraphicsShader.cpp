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
		VkShaderModuleCreateInfo vertexCreateInfo{};
		vertexCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vertexCreateInfo.codeSize = dataVertex.size() * sizeof(uint32_t);
		vertexCreateInfo.pCode = dataVertex.data();

		VkShaderModuleCreateInfo fragmentCreateInfo{};
		fragmentCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fragmentCreateInfo.codeSize = dataFragment.size() * sizeof(uint32_t);
		fragmentCreateInfo.pCode = dataFragment.data();

		const VulkanContextDevice& device = VulkanContextDevice::Get();
		LUCY_VK_ASSERT(vkCreateShaderModule(device.GetLogicalDevice(), &vertexCreateInfo, nullptr, &m_VertexShaderModule));
		LUCY_VK_ASSERT(vkCreateShaderModule(device.GetLogicalDevice(), &fragmentCreateInfo, nullptr, &m_FragmentShaderModule));

		m_ShaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		m_ShaderStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		m_ShaderStageInfos[0].module = m_VertexShaderModule;
		m_ShaderStageInfos[0].pName = "main";

		m_ShaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		m_ShaderStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_ShaderStageInfos[1].module = m_FragmentShaderModule;
		m_ShaderStageInfos[1].pName = "main";
	}

	void VulkanGraphicsShader::Destroy() {
		if (!m_VertexShaderModule || !m_FragmentShaderModule) return;
		const VulkanContextDevice& device = VulkanContextDevice::Get();
		vkDestroyShaderModule(device.GetLogicalDevice(), m_VertexShaderModule, nullptr);
		vkDestroyShaderModule(device.GetLogicalDevice(), m_FragmentShaderModule, nullptr);
	}
}