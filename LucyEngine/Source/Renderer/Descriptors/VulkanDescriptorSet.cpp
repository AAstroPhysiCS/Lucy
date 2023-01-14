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
		const auto& internalInfo = m_CreateInfo.InternalInfo.As<VulkanDescriptorSetCreateInfo>();

		const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, internalInfo->Layout);

		VkDescriptorSetAllocateInfo allocInfo = VulkanAPI::DescriptorSetAllocateInfo(maxFramesInFlight, layouts.data(), internalInfo->Pool->GetVulkanHandle());

		/*
		* cant summarize these since these are stack allocated
		*
		* indicates that this is a variable-sized descriptor binding whose size will be specified when a descriptor set is allocated using this layout.
		* The value of descriptorCount is treated as an upper bound on the size of the binding.
		*/
		std::vector<uint32_t> variableDescCount(allocInfo.descriptorSetCount, MAX_DYNAMIC_DESCRIPTOR_COUNT);
		VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountAllocInfo = VulkanAPI::DescriptorSetVariableDescriptorCountAllocateInfo(allocInfo.descriptorSetCount, variableDescCount.data());

		if (m_CreateInfo.Bindless)
			allocInfo.pNext = &variableCountAllocInfo;

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

		for (Ref<UniformBuffer> buffer : m_UniformBuffers) {
			DescriptorType descriptorType = buffer->GetDescriptorType();

			switch (descriptorType) {
				case DescriptorType::DynamicBuffer:
				case DescriptorType::Buffer: {
					const auto& uniformBuffer = buffer.As<VulkanUniformBuffer>();
					const uint32_t arraySize = buffer->GetArraySize();

					VkDescriptorBufferInfo bufferInfo = VulkanAPI::DescriptorBufferInfo(uniformBuffer->GetVulkanBufferHandle(index), 0, VK_WHOLE_SIZE);
					VkWriteDescriptorSet setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[index], 0, buffer->GetBinding(), arraySize == 0 ? 1 : arraySize, (VkDescriptorType)ConvertDescriptorType(descriptorType), 
																				  &bufferInfo);

					vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);

					uniformBuffer->Clear();
					break;
				}
				case DescriptorType::StorageImage:
				case DescriptorType::Sampler:
				case DescriptorType::SampledImage:
				case DescriptorType::CombinedImageSampler: {
					const auto& uniformImageBuffer = buffer.As<VulkanUniformImageBuffer>();
					auto& imageInfos = uniformImageBuffer->GetImageInfos();

					if (imageInfos.size() == 0)
						break;

					VkWriteDescriptorSet setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[index], 0, buffer->GetBinding(), (uint32_t)imageInfos.size(), (VkDescriptorType)ConvertDescriptorType(descriptorType),
																				  nullptr, imageInfos.data());
					vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
					//cant summarize buffer and combinedimagesampler together since i need to clear the cache
					uniformImageBuffer->Clear();
					break;
				}
				case DescriptorType::Undefined:
					LUCY_ASSERT(false, "Descriptor type is undefined!");
					break;
			}
		}

		for (Ref<SharedStorageBuffer> buffer : m_SharedStorageBuffers) {
			const auto& ssbo = buffer.As<VulkanSharedStorageBuffer>();
			const uint32_t arraySize = buffer->GetArraySize();

			VkDescriptorBufferInfo bufferInfo = VulkanAPI::DescriptorBufferInfo(ssbo->GetVulkanBufferHandle(index), 0, VK_WHOLE_SIZE);

			VkWriteDescriptorSet setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[index], 0, buffer->GetBinding(), arraySize == 0 ? 1 : arraySize, VK_DESCRIPTOR_TYPE_MAX_ENUM,
																		  &bufferInfo);
			switch (buffer->GetDescriptorType()) {
				case DescriptorType::SSBO:
					setWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					break;
				case DescriptorType::SSBODynamic:
					setWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
					LUCY_ASSERT(false, "Dynamic SSBO not yet supported!");
					break;
				case DescriptorType::Undefined:
					LUCY_ASSERT(false, "Descriptor type is undefined!");
					break;
			}

			vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
			buffer->Clear();
		}
	}
}