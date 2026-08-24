#include "lypch.h"
#include "VulkanRenderDeviceUploadManager.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"
#include "Renderer/Commands/VulkanCommandPool.h"

namespace Lucy {
	
	VulkanRenderDeviceUploadManager::VulkanRenderDeviceUploadManager(const Ref<VulkanRenderDevice>& device)
		: m_RenderDevice(device) {
		m_Frames.resize(Renderer::GetMaxFramesInFlight());

		auto& allocator = device->GetAllocator();

		//512mb per frame, should be enough for most cases. If not, TODO: implement a dynamic buffer growth strategy
		constexpr size_t capacityPerFrame = 512ull * 1024ull * 1024ull;

		for (uint32_t frameIndex = 0; frameIndex < m_Frames.size(); frameIndex++) {
			UploadFrame& frame = m_Frames[frameIndex];

			auto allocationInfo = allocator.CreateVulkanBufferVma(MemoryUsage::CPUOnly, capacityPerFrame, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, frame.Buffer, frame.Allocation);

			LUCY_ASSERT(frame.Buffer != VK_NULL_HANDLE);
			LUCY_ASSERT(frame.Allocation != VK_NULL_HANDLE);
			LUCY_ASSERT(allocationInfo.pMappedData != nullptr);

			frame.Mapped = static_cast<uint8_t*>(allocationInfo.pMappedData);
			frame.Capacity = capacityPerFrame;
			frame.Offset = 0;
		}

		m_PendingCopies.reserve(256);
	}

	void VulkanRenderDeviceUploadManager::SyncFrame(size_t frameIndex) {
		if (m_PendingCopies.empty())
			return;

		LUCY_PROFILE_NEW_EVENT("VulkanRenderDeviceUploadManager::SyncFrame");
		const auto DirectCopyBuffer = [this](VkBuffer stagingBuffer, VkBuffer buffer, VkDeviceSize size, std::vector<VkBufferCopy>& copyRegions) {
			LUCY_PROFILE_NEW_EVENT("VulkanRenderDeviceUploadManager::DirectCopyBuffer");
			auto renderDevice = m_RenderDevice->As<VulkanRenderDevice>();
			renderDevice->SubmitImmediateCommand([renderDevice, size, copyRegions, stagingBuffer, buffer](VkCommandBuffer commandBuffer) {
				/*VkMemoryBarrier2 barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
				barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
				barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
				barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

				VkDependencyInfo depInfo{};
				depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
				depInfo.memoryBarrierCount = 1;
				depInfo.pMemoryBarriers = &barrier;*/

				//vkCmdPipelineBarrier2(commandBuffer, &depInfo);
				vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
				//vkCmdPipelineBarrier2(commandBuffer, &depInfo);
			});
		};

		UploadFrame& frame = m_Frames[frameIndex];

		std::unordered_map<VkBuffer, std::vector<VkBufferCopy>> copiesByDestination;
		for (const auto& copy : m_PendingCopies)
			copiesByDestination[copy.Destination].emplace_back(VkBufferCopy{ .srcOffset = copy.SourceOffset, .dstOffset = copy.DestinationOffset, .size = copy.Size });

		for (auto& [destination, regions] : copiesByDestination)
			DirectCopyBuffer(frame.Buffer, destination, regions.size(), regions);
		frame.Offset = 0;
		m_PendingCopies.clear();
	}

	void VulkanRenderDeviceUploadManager::EnqueueUploadBuffer(VkBuffer destination, VkDeviceSize destinationOffset, const void* data, VkDeviceSize size, VkDeviceSize alignment) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDeviceUploadManager::EnqueueUploadBuffer");
		UploadFrame& frame = m_Frames[Renderer::GetCurrentFrameIndex()];

		VkDeviceSize alignedOffset = (frame.Offset + alignment - 1) & ~(alignment - 1);
		LUCY_ASSERT(alignedOffset + size <= frame.Capacity, "Upload ring exhausted!");

		memcpy(frame.Mapped + alignedOffset, data, size);

		m_PendingCopies.push_back({
			.Destination = destination,
			.SourceOffset = alignedOffset,
			.DestinationOffset = destinationOffset,
			.Size = size
		});

		frame.Offset = alignedOffset + size;
	}
	
	void VulkanRenderDeviceUploadManager::Destroy(VulkanAllocator& allocator) {
		for (auto& frame : m_Frames) {
			if (frame.Buffer != VK_NULL_HANDLE)
				allocator.DestroyBuffer(frame.Buffer, frame.Allocation);
		}
	}
}