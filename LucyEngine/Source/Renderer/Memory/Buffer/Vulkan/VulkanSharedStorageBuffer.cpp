#include "lypch.h"
#include "VulkanSharedStorageBuffer.h"

#include "Renderer/Shader/ShaderReflect.h" //for ShaderMemberVariable
#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/Memory/VulkanAllocator.h"

namespace Lucy {

	VulkanSharedStorageBuffer::VulkanSharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo)
		: SharedStorageBuffer(createInfo) {
		if (m_CreateInfo.BufferSize == 0) {
			LUCY_CRITICAL("Size is 0.");
			LUCY_ASSERT(false);
		}

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		const uint32_t maxFramesInFlight = swapChain.GetMaxFramesInFlight();
		m_Buffers.resize(maxFramesInFlight, VK_NULL_HANDLE);
		m_BufferVma.resize(maxFramesInFlight, VK_NULL_HANDLE);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < swapChain.GetMaxFramesInFlight(); i++)
			allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, m_CreateInfo.BufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, m_Buffers[i], m_BufferVma[i]);
	}

	void VulkanSharedStorageBuffer::LoadToGPU() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VulkanAllocator& allocator = VulkanAllocator::Get();
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		void* dataLocal;
		vmaMapMemory(allocator.GetVmaInstance(), m_BufferVma[swapChain.GetCurrentFrameIndex()], &dataLocal);
		memcpy(dataLocal, m_Data.data(), m_Data.size());
		vmaUnmapMemory(allocator.GetVmaInstance(), m_BufferVma[swapChain.GetCurrentFrameIndex()]);
	}

	void VulkanSharedStorageBuffer::DestroyHandle() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < swapChain.GetMaxFramesInFlight(); i++)
			vmaDestroyBuffer(allocator.GetVmaInstance(), m_Buffers[i], m_BufferVma[i]);
	}
}