#pragma once

#include <variant>
#include <vector>

#include "vulkan/vulkan.h"

#include "Semaphore.h"
#include "Device/RenderDevice.h"

namespace Lucy {

	class RenderGraphPass;
	class VulkanImage;

	struct VulkanQueueSubmitInfo {
		VkPipelineStageFlags2 StageMask = VK_PIPELINE_STAGE_2_NONE;
		uint64_t Value = 0;
		VulkanSemaphore Semaphore;
	};

	struct VulkanImageMemoryBarrier {
		Ref<VulkanImage> Image;
		VkImageMemoryBarrier2 Barrier;
	};

	struct VulkanBatchBarrier {
		std::vector<VulkanImageMemoryBarrier> ImageBarriers;
		std::vector<VkBufferMemoryBarrier2> BufferBarriers;
	};

	struct VulkanPassBarrier {
		RenderGraphPass* Pass = nullptr;
		VulkanBatchBarrier Barrier;
	};

	struct VulkanExecutionBatch {
		TargetQueueFamily QueueFamily;
		std::vector<RenderGraphPass*> Passes;
		VulkanBatchBarrier PreBatchBarrier; //release
		std::vector<VulkanPassBarrier> PassBarriers; //barriers inside of a single batch
		VulkanBatchBarrier PostBatchBarrier; //acquire
		std::vector<VulkanQueueSubmitInfo> Waits;
		std::vector<VulkanQueueSubmitInfo> Signals;
	};

	// TODO: DELETE, is for testing
	struct D3D12ExecutionBatch {

	};

	using ExecutionBatchID = size_t;

	struct ExecutionBatch : public std::variant<VulkanExecutionBatch, D3D12ExecutionBatch> {
		using std::variant<VulkanExecutionBatch, D3D12ExecutionBatch>::variant;

		ExecutionBatchID ID = 0;
		
		inline bool IsVulkanBatch() const { return std::holds_alternative<VulkanExecutionBatch>(*this); }
		inline bool IsD3D12Batch() const { return std::holds_alternative<D3D12ExecutionBatch>(*this); }

		inline VulkanExecutionBatch& AsVulkanBatch() { return std::get<VulkanExecutionBatch>(*this); }
		inline const VulkanExecutionBatch& AsVulkanBatch() const { return std::get<VulkanExecutionBatch>(*this); }
		inline const D3D12ExecutionBatch& AsD3D12Batch() const { return std::get<D3D12ExecutionBatch>(*this); }
	};
}
