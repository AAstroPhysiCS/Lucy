#pragma once

#include "vulkan/vulkan.h"

#include "Memory.h"

namespace Lucy {

	class VulkanRenderDevice;
	class VulkanAllocator;
	class VulkanTransientCommandPool;

	class VulkanRenderDeviceUploadManager final {
	public:
		VulkanRenderDeviceUploadManager(const Ref<VulkanRenderDevice>& device);
		~VulkanRenderDeviceUploadManager() = default;

		VulkanRenderDeviceUploadManager(const VulkanRenderDeviceUploadManager&) = delete;
		VulkanRenderDeviceUploadManager& operator=(const VulkanRenderDeviceUploadManager&) = delete;
		VulkanRenderDeviceUploadManager(VulkanRenderDeviceUploadManager&&) = delete;
		VulkanRenderDeviceUploadManager& operator=(VulkanRenderDeviceUploadManager&&) = delete;

		void SyncFrame(size_t frameIndex);

		void EnqueueUploadBuffer(VkBuffer destination, VkDeviceSize destinationOffset, const void* data, VkDeviceSize size, VkDeviceSize alignment = 16);
		void Destroy(VulkanAllocator& allocator);
	private:
		struct PendingBufferCopy {
			VkBuffer Destination = VK_NULL_HANDLE;

			VkDeviceSize SourceOffset = 0;
			VkDeviceSize DestinationOffset = 0;
			VkDeviceSize Size = 0;
		};

		struct UploadFrame {
			VkBuffer Buffer = VK_NULL_HANDLE;
			VmaAllocation Allocation = VK_NULL_HANDLE;

			uint8_t* Mapped = nullptr;

			VkDeviceSize Capacity = 0;
			VkDeviceSize Offset = 0;

			std::vector<PendingBufferCopy> PendingCopies;
		};

		std::vector<UploadFrame> m_Frames;

		Ref<VulkanRenderDevice> m_RenderDevice = nullptr;
	};
}
