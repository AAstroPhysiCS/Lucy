#pragma once

#include "Buffer.h"
#include "vulkan/vulkan.h"

#include "glm/glm.hpp"

namespace Lucy {

	class VulkanPushConstant : public Buffer<uint8_t> {
	public:
		VulkanPushConstant() = default;
		VulkanPushConstant(const std::string& name, uint32_t size, uint32_t offset, VkShaderStageFlags shaderStage);
		virtual ~VulkanPushConstant() = default;

		void Bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const;

		inline std::string GetName() const { return m_Name; }
		inline VkPushConstantRange GetHandle() const { return m_Handle; }
	private:
		std::string m_Name;
		uint32_t m_Size = 0;
		uint32_t m_Offset = 0;
		VkShaderStageFlags m_ShaderStage;

		VkPushConstantRange m_Handle;
	};
}

