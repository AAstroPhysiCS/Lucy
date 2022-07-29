#pragma once

#include "RenderDeviceCommandList.h"

namespace Lucy {

	class VulkanRenderDeviceCommandList final : public RenderDeviceCommandList {
	public:
		VulkanRenderDeviceCommandList() = default;
		virtual ~VulkanRenderDeviceCommandList() = default;

		void Init() final override;
		void ExecuteSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func);
	private:
		void ExecuteCommandQueue() final override;
	};
}

