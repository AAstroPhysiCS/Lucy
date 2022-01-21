#pragma once

#include "RenderPass.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanRenderPass : public RenderPass {
	public:
		VulkanRenderPass(RenderPassSpecification& specs);
		virtual ~VulkanRenderPass() = default;

		void Begin(RenderPassBeginInfo& info);
		void End(RenderPassEndInfo& info);
		void Recreate();
		void Destroy();

		inline VkRenderPass GetVulkanHandle() { return m_RenderPass; }
	private:
		void Create();

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};
}