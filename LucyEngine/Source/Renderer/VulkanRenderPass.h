#pragma once

#include "RenderPass.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanRenderPass : public RenderPass {
	public:
		VulkanRenderPass(const RenderPassCreateInfo& createInfo);
		virtual ~VulkanRenderPass() = default;

		void Begin(RenderPassBeginInfo& info) final override;
		void End() final override;
		void Destroy() final override;
		void Recreate() final override;

		inline VkRenderPass GetVulkanHandle() { return m_RenderPass; }
		inline uint32_t GetAttachmentCount() { return m_AttachmentCount; }
		inline bool IsDepthBuffered() { return m_DepthBuffered; }
	private:
		void Create();

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		uint32_t m_AttachmentCount = 0;
		bool m_DepthBuffered = false;

		//just a helper member variable, to save the commandbuffer that was given in begininfo
		VkCommandBuffer m_BoundedCommandBuffer = VK_NULL_HANDLE;
	};
}