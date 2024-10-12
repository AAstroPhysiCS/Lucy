#pragma once

#include "RenderPass.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanRenderDevice;

	struct VulkanRenderPassBeginInfo {
		VkFramebuffer VulkanFrameBuffer = nullptr;
		VkCommandBuffer CommandBuffer = nullptr;
		uint32_t Width = 0;
		uint32_t Height = 0;
	};

	class VulkanRenderPass : public RenderPass {
	public:
		VulkanRenderPass(const RenderPassCreateInfo& createInfo, const Ref<VulkanRenderDevice>& vulkanDevice);
		virtual ~VulkanRenderPass() = default;

		void RTBegin(VulkanRenderPassBeginInfo& info);
		void RTEnd();
		void RTRecreate() final override;

		inline VkRenderPass GetVulkanHandle() { return m_RenderPass; }
		inline uint32_t GetAttachmentCount() { return m_AttachmentCount; }
		inline uint32_t GetColorAttachmentCount() { return m_ColorAttachmentCount; }
	private:
		void RTCreate();
		void RTDestroyResource() final override;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		uint32_t m_AttachmentCount = 0;
		uint32_t m_ColorAttachmentCount = 0;

		//just a helper member variable, to save the commandbuffer that was given in begininfo
		VkCommandBuffer m_BoundedCommandBuffer = VK_NULL_HANDLE;
		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;

		friend class VulkanSwapChain; //Exception, for ImGui initialization.
	};
}