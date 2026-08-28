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
			frame.PendingCopies.reserve(256);
		}
	}

	void VulkanRenderDeviceUploadManager::SyncFrame(size_t frameIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDeviceUploadManager::SyncFrame");
		UploadFrame& frame = m_Frames[frameIndex];
		if (frame.PendingCopies.empty())
			return;

		auto renderDevice = m_RenderDevice->As<VulkanRenderDevice>();
		renderDevice->SubmitImmediateCommand([&frame](VkCommandBuffer commandBuffer) {
			for (const PendingBufferCopy& copy : frame.PendingCopies) {
				VkBufferCopy region { .srcOffset = copy.SourceOffset, .dstOffset = copy.DestinationOffset, .size = copy.Size };
				vkCmdCopyBuffer(commandBuffer, frame.Buffer, copy.Destination, 1, &region);
			}
		});

		frame.Offset = 0;
		frame.PendingCopies.clear();
	}

	void VulkanRenderDeviceUploadManager::EnqueueUploadBuffer(VkBuffer destination, VkDeviceSize destinationOffset, const void* data, VkDeviceSize size, VkDeviceSize alignment) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDeviceUploadManager::EnqueueUploadBuffer");
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		UploadFrame& frame = m_Frames[frameIndex];

		VkDeviceSize alignedOffset = (frame.Offset + alignment - 1) & ~(alignment - 1);
		LUCY_ASSERT(alignedOffset + size <= frame.Capacity, "Upload ring exhausted!");

		memcpy(frame.Mapped + alignedOffset, data, size);

		frame.PendingCopies.push_back({
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