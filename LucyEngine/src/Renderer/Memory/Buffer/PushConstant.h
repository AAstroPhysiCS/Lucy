#pragma once

#include "Buffer.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	struct PushConstantBindInfo {
		VkCommandBuffer CommandBuffer;
		VkPipelineLayout PipelineLayout;
	};

	class PushConstant : public Buffer<void*> {
	public:
		PushConstant() = default;
		PushConstant(const std::string& name, uint32_t size, uint32_t offset, VkShaderStageFlags shaderStage);
		virtual ~PushConstant() = default;

		void Bind(PushConstantBindInfo& info) const ;

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

