#include "lypch.h"
#include "PushConstant.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanPushConstant::VulkanPushConstant(const std::string& name, uint32_t size, uint32_t offset, VkShaderStageFlags shaderStage)
		: m_Name(name), m_Size(size), m_Offset(offset), m_ShaderStage(shaderStage) {
		m_Handle.size = m_Size;
		m_Handle.offset = m_Offset;
		m_Handle.stageFlags = shaderStage;

		Resize(m_Size);
	}

	void VulkanPushConstant::RTBind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const {
		vkCmdPushConstants(commandBuffer, layout, m_ShaderStage, m_Offset, m_Size, m_Data.data());
	}
}