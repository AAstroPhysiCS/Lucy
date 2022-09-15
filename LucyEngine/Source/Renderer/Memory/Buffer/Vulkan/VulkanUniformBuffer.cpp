#include "lypch.h"
#include "VulkanUniformBuffer.h"

#include "Renderer/Shader/ShaderReflect.h" //for ShaderMemberVariable
#include "Renderer/Context/VulkanContextDevice.h"
#include "Renderer/Memory/VulkanAllocator.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanUniformBuffer::VulkanUniformBuffer(UniformBufferCreateInfo& createInfo)
		: UniformBuffer(createInfo) {
		if (m_CreateInfo.BufferSize == 0) {
			LUCY_CRITICAL("Size is 0.");
			LUCY_ASSERT(false);
		}

		const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();
		m_Buffers.resize(maxFramesInFlight, VK_NULL_HANDLE);
		m_BufferVma.resize(maxFramesInFlight, VK_NULL_HANDLE);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < maxFramesInFlight; i++)
			allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, m_CreateInfo.BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_Buffers[i], m_BufferVma[i]);
	}

	void VulkanUniformBuffer::LoadToGPU() {
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();
		VulkanAllocator& allocator = VulkanAllocator::Get();

		void* dataLocal;
		allocator.MapMemory(m_BufferVma[Renderer::GetCurrentFrameIndex()], dataLocal);
		memcpy(dataLocal, m_Data.data(), m_Data.size());
		allocator.UnmapMemory(m_BufferVma[Renderer::GetCurrentFrameIndex()]);
	}

	void VulkanUniformBuffer::DestroyHandle() {
		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < Renderer::GetMaxFramesInFlight(); i++)
			allocator.DestroyBuffer(m_Buffers[i], m_BufferVma[i]);
	}

	VulkanUniformImageBuffer::VulkanUniformImageBuffer(UniformBufferCreateInfo& createInfo)
		: UniformBuffer(createInfo) {
	}

	void VulkanUniformImageBuffer::LoadToGPU() {
		//empty, since we really dont "upload" it to the gpu with a given buffer
	}

	uint32_t VulkanUniformImageBuffer::BindImage(Ref<VulkanImage2D> image) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = image->GetCurrentLayout();
		imageInfo.imageView = image->GetImageView().GetVulkanHandle();
		imageInfo.sampler = image->GetImageView().GetSampler();

		m_ImageInfos.push_back(imageInfo);
		return m_ImageInfos.size() - 1;
	}

	uint32_t VulkanUniformImageBuffer::BindImage(VkImageView imageView, VkImageLayout layout, VkSampler sampler) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = layout;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		m_ImageInfos.push_back(imageInfo);
		return m_ImageInfos.size() - 1;
	}

	void VulkanUniformImageBuffer::Clear() {
		m_ImageInfos.clear();
	}

	void VulkanUniformImageBuffer::DestroyHandle() {
		//Empty, images are being deleted in the material class
	}
}