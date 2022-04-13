#pragma once

#include "RenderPass.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanRenderPass : public RenderPass {
	public:
		VulkanRenderPass(const RenderPassSpecification& specs);
		virtual ~VulkanRenderPass() = default;

		void Begin(RenderPassBeginInfo& info) override;
		void End(RenderPassEndInfo& info) override;
		void Destroy() override;
		void Recreate() override;

		inline VkRenderPass GetVulkanHandle() { return m_RenderPass; }
	private:
		void Create();

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};
}