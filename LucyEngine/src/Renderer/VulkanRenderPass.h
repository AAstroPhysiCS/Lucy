#pragma once

#include "RenderPass.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanRenderPass : public RenderPass {
	public:
		VulkanRenderPass(const RenderPassSpecification& specs);
		virtual ~VulkanRenderPass() = default;

		void Begin(RenderPassBeginInfo& info) override;
		void End() override;
		void Destroy() override;
		void Recreate() override;

		inline VkRenderPass GetVulkanHandle() { return m_RenderPass; }
	private:
		void Create();

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		//just a helper member variable, to save the commandbuffer that was given in begininfo
		VkCommandBuffer m_BoundedCommandBuffer = VK_NULL_HANDLE;
	};
}