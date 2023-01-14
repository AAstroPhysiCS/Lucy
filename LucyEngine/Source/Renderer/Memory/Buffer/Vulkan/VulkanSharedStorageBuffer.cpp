#include "lypch.h"
#include "VulkanSharedStorageBuffer.h"

#include "Renderer/Shader/ShaderReflect.h" //for ShaderMemberVariable
#include "Renderer/Context/VulkanContextDevice.h"
#include "Renderer/Memory/VulkanAllocator.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanSharedStorageBuffer::VulkanSharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo)
		: SharedStorageBuffer(createInfo) {
		LUCY_ASSERT(m_CreateInfo.BufferSize != 0, "The size of SSBO is 0.");

		const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();
		m_Buffers.resize(maxFramesInFlight, VK_NULL_HANDLE);
		m_BufferVma.resize(maxFramesInFlight, VK_NULL_HANDLE);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < maxFramesInFlight; i++)
			allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, m_CreateInfo.BufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, m_Buffers[i], m_BufferVma[i]);
	}

	void VulkanSharedStorageBuffer::LoadToGPU() {
		VulkanAllocator& allocator = VulkanAllocator::Get();
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		void* dataLocal;
		allocator.MapMemory(m_BufferVma[Renderer::GetCurrentFrameIndex()], dataLocal);
		memcpy(dataLocal, m_Data.data(), m_Data.size());
		allocator.UnmapMemory(m_BufferVma[Renderer::GetCurrentFrameIndex()]);
	}

	void VulkanSharedStorageBuffer::DestroyHandle() {
		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < Renderer::GetMaxFramesInFlight(); i++)
			allocator.DestroyBuffer(m_Buffers[i], m_BufferVma[i]);
	}
}