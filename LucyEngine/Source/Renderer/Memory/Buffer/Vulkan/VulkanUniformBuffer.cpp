#include "lypch.h"
#include "VulkanUniformBuffer.h"

#include "Renderer/Shader/ShaderReflect.h" //for ShaderMemberVariable
#include "Renderer/Device/VulkanRenderDevice.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanUniformBuffer::VulkanUniformBuffer(const UniformBufferCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: UniformBuffer(createInfo), m_VulkanDevice(device) {
		LUCY_ASSERT(m_CreateInfo.BufferSize != 0, "The size of UBO is 0.");

		const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();
		m_Buffers.resize(maxFramesInFlight, VK_NULL_HANDLE);
		m_BufferVma.resize(maxFramesInFlight, VK_NULL_HANDLE);

		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		for (uint32_t i = 0; i < maxFramesInFlight; i++)
			allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, m_CreateInfo.BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_Buffers[i], m_BufferVma[i]);
	}

	void VulkanUniformBuffer::RTLoadToDevice() {
		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();

		void* dataLocal;
		allocator.MapMemory(m_BufferVma[Renderer::GetCurrentFrameIndex()], dataLocal);
		memcpy(dataLocal, m_Data.data(), m_Data.size());
		allocator.UnmapMemory(m_BufferVma[Renderer::GetCurrentFrameIndex()]);
	}

	void VulkanUniformBuffer::RTDestroyResource() {
		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		for (uint32_t i = 0; i < Renderer::GetMaxFramesInFlight(); i++)
			allocator.DestroyBuffer(m_Buffers[i], m_BufferVma[i]);
	}
}