#include "lypch.h"
#include "VulkanDescriptorSet.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanUniformBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanSharedStorageBuffer.h"
#include "Renderer/Context/VulkanContextDevice.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanDescriptorSet::VulkanDescriptorSet(const DescriptorSetCreateInfo& createInfo)
		: DescriptorSet(createInfo) {
		Create();
	}

	void VulkanDescriptorSet::Create() {
		auto& internalInfo = m_CreateInfo.InternalInfo.As<VulkanDescriptorSetCreateInfo>();

		const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, internalInfo->Layout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pSetLayouts = layouts.data();
		allocInfo.descriptorPool = internalInfo->Pool->GetVulkanHandle();
		allocInfo.descriptorSetCount = maxFramesInFlight;

		/*
		* cant summarize these since these are stack allocated
		*
		* indicates that this is a variable-sized descriptor binding whose size will be specified when a descriptor set is allocated using this layout.
		* The value of descriptorCount is treated as an upper bound on the size of the binding.
		*/
		std::vector<uint32_t> variableDescCount(allocInfo.descriptorSetCount, MAX_DYNAMIC_DESCRIPTOR_COUNT);
		VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountAllocInfo{};

		if (m_CreateInfo.Bindless) {
			variableCountAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
			variableCountAllocInfo.descriptorSetCount = allocInfo.descriptorSetCount;
			variableCountAllocInfo.pDescriptorCounts = variableDescCount.data();

			allocInfo.pNext = &variableCountAllocInfo;
		}

		m_DescriptorSets.resize(maxFramesInFlight);
		LUCY_VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data()));
	}

	void VulkanDescriptorSet::Bind(const VulkanDescriptorSetBindInfo& bindInfo) {
		vkCmdBindDescriptorSets(bindInfo.CommandBuffer, bindInfo.PipelineBindPoint, bindInfo.PipelineLayout, m_CreateInfo.SetIndex, 1, &m_DescriptorSets[Renderer::GetCurrentFrameIndex()], 0, nullptr);
	}

	void VulkanDescriptorSet::Update() {
		LUCY_PROFILE_NEW_EVENT("VulkanDescriptorSet::Update");

		for (Ref<UniformBuffer>& buffer : m_UniformBuffers)
			buffer->LoadToGPU();

		for (Ref<SharedStorageBuffer>& buffer : m_SharedStorageBuffers)
			buffer->LoadToGPU();

		const uint32_t index = Renderer::GetCurrentFrameIndex();

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		VkWriteDescriptorSet setWrite{};
		setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrite.dstSet = m_DescriptorSets[index];
		setWrite.dstArrayElement = 0;
		setWrite.pTexelBufferView = nullptr;

		for (Ref<UniformBuffer> buffer : m_UniformBuffers) {
			setWrite.dstBinding = buffer->GetBinding();

			switch (buffer->GetDescriptorType()) {
				case DescriptorType::Buffer: {
					auto& uniformBuffer = buffer.As<VulkanUniformBuffer>();
					const uint32_t arraySize = buffer->GetArraySize();

					VkDescriptorBufferInfo bufferInfo{};
					bufferInfo.buffer = uniformBuffer->GetVulkanBufferHandle(index);
					bufferInfo.offset = 0;
					bufferInfo.range = VK_WHOLE_SIZE;

					setWrite.pBufferInfo = &bufferInfo;
					setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					setWrite.descriptorCount = arraySize == 0 ? 1 : arraySize;

					vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);

					uniformBuffer->Clear();
					break;
				}
				case DescriptorType::Sampler: //TODO: Do these stuff, if needed obv
				case DescriptorType::SampledImage:
				case DescriptorType::CombinedImageSampler: {
					auto& uniformImageBuffer = buffer.As<VulkanUniformImageBuffer>();
					auto& imageInfos = uniformImageBuffer->GetImageInfos();

					if (imageInfos.size() == 0)
						break;

					setWrite.pImageInfo = imageInfos.data();
					setWrite.descriptorCount = imageInfos.size();
					setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

					vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
					//cant summarize buffer and combinedimagesampler together since i need to clear the cache
					uniformImageBuffer->Clear();
					break;
				}
				case DescriptorType::DynamicBuffer:
					LUCY_CRITICAL("Not implemented yet!");
					LUCY_ASSERT(false);
					break;
				case DescriptorType::Undefined:
					LUCY_CRITICAL("Descriptor type is undefined!");
					LUCY_ASSERT(false);
					break;
			}
		}

		for (Ref<SharedStorageBuffer> buffer : m_SharedStorageBuffers) {
			auto& ssbo = buffer.As<VulkanSharedStorageBuffer>();
			const uint32_t arraySize = buffer->GetArraySize();

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = ssbo->GetVulkanBufferHandle(index);
			bufferInfo.offset = 0;
			bufferInfo.range = VK_WHOLE_SIZE;

			setWrite.dstBinding = buffer->GetBinding();
			setWrite.pBufferInfo = &bufferInfo;
			setWrite.descriptorCount = arraySize == 0 ? 1 : arraySize;

			switch (buffer->GetDescriptorType()) {
				case DescriptorType::SSBO:
					setWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					break;
				case DescriptorType::SSBODynamic:
					setWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
					LUCY_CRITICAL("Not implemented yet!");
					LUCY_ASSERT(false);
					break;
				case DescriptorType::Undefined:
					LUCY_CRITICAL("Descriptor type is undefined!");
					LUCY_ASSERT(false);
					break;
			}

			vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
			buffer->Clear();
		}
	}
}