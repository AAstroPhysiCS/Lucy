#include "lypch.h"
#include "VulkanDescriptorSet.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanUniformBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanSharedStorageBuffer.h"

#include "Renderer/Shader/VulkanUniformImageSampler.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanDescriptorSet::VulkanDescriptorSet(const DescriptorSetCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: DescriptorSet(createInfo), m_VulkanDevice(device) {
		RTCreate();
	}

	void VulkanDescriptorSet::RTCreate() {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		
		for (const auto& block : m_CreateInfo.ShaderUniformBlocks) {
			switch (block.Type) {
				using enum Lucy::DescriptorType;
				case SSBODynamic:
				case SSBO: {
					SharedStorageBufferCreateInfo createInfo;
					createInfo.Name = block.Name;
					createInfo.Binding = block.Binding;
					createInfo.Type = block.Type;
					createInfo.BufferSize = MAX_DYNAMICALLY_ALLOCATED_BUFFER_SIZE;
					createInfo.ArraySize = block.ArraySize;
					createInfo.ShaderMemberVariables = block.Members;

					AddSharedStorageBuffer(createInfo.Name, m_VulkanDevice->CreateSharedStorageBuffer(createInfo));
					break;
				}
				case SampledImage:
				case Sampler:
				case CombinedImageSampler:
				case StorageImage: {
					m_UniformImageSamplers.try_emplace(block.Name, Memory::CreateRef<VulkanUniformImageSampler>(block.Binding, block.Name, block.Type));
					break;
				}
				case Buffer:
				case DynamicBuffer: {
					UniformBufferCreateInfo createInfo;
					createInfo.Name = block.Name;
					createInfo.Binding = block.Binding;
					createInfo.Type = block.Type;
					createInfo.BufferSize = block.BufferSize;
					createInfo.ArraySize = block.ArraySize;
					createInfo.ShaderMemberVariables = block.Members;

					AddUniformBuffer(createInfo.Name, m_VulkanDevice->CreateUniformBuffer(createInfo));
					break;
				}
				default: LUCY_ASSERT(false);
			}
		}
	}

	void VulkanDescriptorSet::RTBind(const VulkanDescriptorSetBindInfo& bindInfo) {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		vkCmdBindDescriptorSets(bindInfo.CommandBuffer, bindInfo.PipelineBindPoint, bindInfo.PipelineLayout, m_CreateInfo.SetIndex, 1, &m_DescriptorSets[Renderer::GetCurrentFrameIndex()], 0, nullptr);
	}

	void VulkanDescriptorSet::RTBake(const Ref<VulkanDescriptorPool>& descriptorPool) {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		
		const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();
		VkDevice device = m_VulkanDevice->GetLogicalDevice();

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		std::vector<bool> isBindlessVector;

		for (auto& buffer : m_CreateInfo.ShaderUniformBlocks) {
			VkDescriptorSetLayoutBinding binding = VulkanAPI::DescriptorSetLayoutBinding(buffer.Binding, buffer.ArraySize == 0 ? 1 : buffer.ArraySize, buffer.Type, buffer.StageFlag);
			isBindlessVector.push_back(buffer.DynamicallyAllocated); //the set is bindless if true

			if (buffer.DynamicallyAllocated) {
				buffer.ArraySize = MAX_DYNAMIC_DESCRIPTOR_COUNT;
				binding.descriptorCount = buffer.ArraySize;
			}

			layoutBindings.push_back(binding);
		}

		VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = VulkanAPI::DescriptorSetCreateInfo((uint32_t)layoutBindings.size(), layoutBindings.data());

		/*
		* indicates that this is a variable-sized descriptor binding whose size will be specified when a descriptor set is allocated using this layout.
		* The value of descriptorCount is treated as an upper bound on the size of the binding.
		*/

		std::vector<VkDescriptorBindingFlags> bindlessDescriptorFlags;

		for (uint32_t i = 0; i < isBindlessVector.size(); i++) {
			if (isBindlessVector[i]) {
				bindlessDescriptorFlags.push_back(VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
				continue;
			}
			bindlessDescriptorFlags.push_back(0); //yes, we really need the 0.
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfo extendedLayoutInfo = VulkanAPI::DescriptorSetLayoutBindingFlagsCreateInfo((uint32_t)layoutBindings.size(), bindlessDescriptorFlags.data());
		descriptorLayoutInfo.pNext = &extendedLayoutInfo;

		LUCY_VK_ASSERT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &m_DescriptorSetLayout));

		/*
		* if any of the bindings are bindless.
		* we are assuming that the set is being used ONLY for bindless descriptors
		* which means, we can't mix a binding which DOESN'T use bindless with a binding that USES bindless
		*/
		bool bindless = std::ranges::any_of(isBindlessVector.begin(), isBindlessVector.end(), [](bool out) { return out; });

		std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, m_DescriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo = VulkanAPI::DescriptorSetAllocateInfo(maxFramesInFlight, layouts.data(), descriptorPool->GetVulkanHandle());

		/*
		* cant summarize these since these are stack allocated
		*
		* indicates that this is a variable-sized descriptor binding whose size will be specified when a descriptor set is allocated using this layout.
		* The value of descriptorCount is treated as an upper bound on the size of the binding.
		*/
		std::vector<uint32_t> variableDescCount(allocInfo.descriptorSetCount, MAX_DYNAMIC_DESCRIPTOR_COUNT);
		VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountAllocInfo = VulkanAPI::DescriptorSetVariableDescriptorCountAllocateInfo(allocInfo.descriptorSetCount, variableDescCount.data());

		if (bindless)
			allocInfo.pNext = &variableCountAllocInfo;

		m_DescriptorSets.resize(maxFramesInFlight);
		LUCY_VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data()));
	}

	void VulkanDescriptorSet::RTUpdate() {
		LUCY_ASSERT(Renderer::IsOnRenderThread());

		LUCY_ASSERT(!m_DescriptorSets.empty());
		LUCY_PROFILE_NEW_EVENT("VulkanDescriptorSet::Update");

		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		VkDevice device = m_VulkanDevice->GetLogicalDevice();

		for (RenderResourceHandle bufferHandle : GetAllUniformBufferHandles() | std::views::values) {
			const auto& uniformBuffer = Renderer::AccessResource<VulkanUniformBuffer>(bufferHandle);
			if (!uniformBuffer)
				continue;
			uniformBuffer->RTLoadToDevice();

			DescriptorType descriptorType = uniformBuffer->GetDescriptorType();
			switch (descriptorType) {
				case DescriptorType::DynamicBuffer:
				case DescriptorType::Buffer: {
					const uint32_t arraySize = uniformBuffer->GetArraySize();

					VkDescriptorBufferInfo bufferInfo = VulkanAPI::DescriptorBufferInfo(uniformBuffer->GetVulkanBufferHandle(frameIndex), 0, VK_WHOLE_SIZE);
					VkWriteDescriptorSet setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[frameIndex], 0, 
						uniformBuffer->GetBinding(), arraySize == 0 ? 1 : arraySize, 
						(VkDescriptorType)ConvertDescriptorType(descriptorType), &bufferInfo);

					vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);

					uniformBuffer->Clear();
					break;
				}
				case DescriptorType::Undefined:
					LUCY_ASSERT(false, "Descriptor type is undefined!");
					break;
			}
		}

		for (RenderResourceHandle bufferHandle : GetAllSharedStorageBufferHandles() | std::views::values) {
			const auto& ssbo = m_VulkanDevice->AccessResource<SharedStorageBuffer>(bufferHandle)->As<VulkanSharedStorageBuffer>();
			ssbo->RTLoadToDevice();

			const uint32_t arraySize = ssbo->GetArraySize();

			VkDescriptorBufferInfo bufferInfo = VulkanAPI::DescriptorBufferInfo(ssbo->GetVulkanBufferHandle(frameIndex), 0, VK_WHOLE_SIZE);

			VkWriteDescriptorSet setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[frameIndex], 0, ssbo->GetBinding(), arraySize == 0 ? 1 : arraySize, VK_DESCRIPTOR_TYPE_MAX_ENUM,
																		  &bufferInfo);
			switch (ssbo->GetDescriptorType()) {
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
			ssbo->Clear();
		}

		if (m_UniformImageSamplers.empty())
			return;

		for (const Ref<VulkanUniformImageSampler>& sampler : m_UniformImageSamplers | std::views::values) {
			auto& imageInfos = sampler->ImageInfos;
			if (imageInfos.empty())
				break;

			VkWriteDescriptorSet setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[frameIndex], 0, sampler->Binding, (uint32_t)imageInfos.size(), (VkDescriptorType)ConvertDescriptorType(sampler->DescriptorType),
																		  nullptr, imageInfos.data());
			vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);

			imageInfos.clear();
		}
	}

	Ref<VulkanUniformImageSampler> VulkanDescriptorSet::GetVulkanImageSampler(const std::string& imageBufferName) {
		if (!m_UniformImageSamplers.contains(imageBufferName))
			return nullptr;
		return m_UniformImageSamplers.at(imageBufferName);
	}

	void VulkanDescriptorSet::RTDestroyResource() {
		LUCY_ASSERT(Renderer::IsOnRenderThread());

		for (auto bufferHandle : GetAllUniformBufferHandles() | std::views::values)
			m_VulkanDevice->RTDestroyResource(bufferHandle);
		for (auto bufferHandle : GetAllSharedStorageBufferHandles() | std::views::values)
			m_VulkanDevice->RTDestroyResource(bufferHandle);

		vkDestroyDescriptorSetLayout(m_VulkanDevice->GetLogicalDevice(), m_DescriptorSetLayout, nullptr);
	}
}