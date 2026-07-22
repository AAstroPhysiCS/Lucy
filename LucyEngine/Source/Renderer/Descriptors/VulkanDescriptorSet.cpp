#include "lypch.h"
#include "VulkanDescriptorSet.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanUniformBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanSharedStorageBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanDeviceAddressBuffer.h"

#include "Renderer/Pipeline/VulkanImageSamplerBindingInfo.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanDescriptorSet::VulkanDescriptorSet(const DescriptorSetCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: DescriptorSet(createInfo), m_VulkanDevice(device) {
		RTCreate();
	}

	void VulkanDescriptorSet::RTCreate() {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		const auto& scene = m_VulkanDevice->GetScene();

		for (const auto& variable : m_CreateInfo.ShaderVariables) {
			switch (variable.Type.Shape) {
				case DescriptorBaseShape::RWSharedStorageBuffer:
				case DescriptorBaseShape::SharedStorageBuffer: {
					SharedStorageBufferCreateInfo createInfo;
					createInfo.Name = variable.Name;
					createInfo.Binding = variable.Binding;
					createInfo.Type = variable.Type;
					createInfo.BufferSize = variable.BufferSize;
					createInfo.ArraySize = variable.ArraySize;
					createInfo.ShaderChildrenVariables = variable.Layout.Children;
					createInfo.ShaderMemberVariables = variable.Layout.Members;

					AddSharedStorageBuffer(createInfo.Name, m_VulkanDevice->CreateSharedStorageBuffer(createInfo));
					break;
				}
				case DescriptorBaseShape::ConstantBuffer: {
					UniformBufferCreateInfo createInfo;
					createInfo.Name = variable.Name;
					createInfo.Binding = variable.Binding;
					createInfo.Type = variable.Type;
					createInfo.BufferSize = variable.BufferSize;
					createInfo.ArraySize = variable.ArraySize;
					createInfo.ShaderChildrenVariables = variable.Layout.Children;
					createInfo.ShaderMemberVariables = variable.Layout.Members;

					AddUniformBuffer(createInfo.Name, m_VulkanDevice->CreateUniformBuffer(createInfo));
					break;
				}
				case DescriptorBaseShape::RWTexture2D:
				case DescriptorBaseShape::RWTexture2DArray:
				case DescriptorBaseShape::RWTexture3D:
				case DescriptorBaseShape::Texture2D:
				case DescriptorBaseShape::Texture2DArray:
				case DescriptorBaseShape::TextureCube:
				case DescriptorBaseShape::TextureCubeArray:
				case DescriptorBaseShape::Texture3D:
				case DescriptorBaseShape::SampledImage:
				case DescriptorBaseShape::SampledImageArray:
				case DescriptorBaseShape::Sampler: {
					m_ImageSamplerBindingInfos.try_emplace(variable.Name, VulkanImageSamplerBindingInfo{ variable.Binding, variable.Name, variable.Type });
					break;
				}
				default: {
					LUCY_CRITICAL("Shader variable '{0}' isn't being parsed by descriptor sets, because of its descriptor base shape", variable.Name);
					break;
				}
			}
		}
	}

	void VulkanDescriptorSet::RTBind(const VulkanDescriptorSetBindInfo& bindInfo) {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		//if the descriptor set needs to be updated per frame (aka if the descriptor set is non-global)
		if (m_CreateInfo.Count == Renderer::GetMaxFramesInFlight())
			vkCmdBindDescriptorSets(bindInfo.CommandBuffer, bindInfo.PipelineBindPoint, bindInfo.PipelineLayout, m_CreateInfo.SetIndex, 1, &m_DescriptorSets[Renderer::GetCurrentFrameIndex()], 0, nullptr);
		else
			vkCmdBindDescriptorSets(bindInfo.CommandBuffer, bindInfo.PipelineBindPoint, bindInfo.PipelineLayout, m_CreateInfo.SetIndex, 1, &m_DescriptorSets[0], 0, nullptr);
	}

	void VulkanDescriptorSet::RTBake(const Ref<VulkanDescriptorPool>& descriptorPool) {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		
		const uint32_t maxFramesInFlight = m_CreateInfo.Count;
		VkDevice device = m_VulkanDevice->GetLogicalDevice();

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		std::vector<bool> isBindlessVector;

		for (auto& buffer : m_CreateInfo.ShaderVariables) {
			VkDescriptorSetLayoutBinding binding = VulkanAPI::DescriptorSetLayoutBinding(buffer.Binding, buffer.ArraySize == 0 ? 1 : buffer.ArraySize, buffer.Type, buffer.StageFlag);
			isBindlessVector.push_back(buffer.DynamicallyAllocated); //the set is bindless if true

			if (buffer.DynamicallyAllocated) {
				buffer.ArraySize = 256;
				binding.descriptorCount = buffer.ArraySize;
			}

			layoutBindings.push_back(binding);
		}

		VkDescriptorSetLayoutCreateFlags layoutFlags = 0;
		const bool isBindless = std::ranges::any_of(isBindlessVector, [](bool value) {
			return value;
		});

		if (isBindless)
			layoutFlags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

		VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = VulkanAPI::DescriptorSetCreateInfo((uint32_t)layoutBindings.size(), layoutBindings.data(), layoutFlags);

		/*
		* indicates that this is a variable-sized descriptor binding whose size will be specified when a descriptor set is allocated using this layout.
		* The value of descriptorCount is treated as an upper bound on the size of the binding.
		*/

		std::vector<VkDescriptorBindingFlags> bindlessDescriptorFlags;

		for (uint32_t i = 0; i < isBindlessVector.size(); i++) {
			if (isBindlessVector[i]) {
				bindlessDescriptorFlags.push_back(VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
				continue;
			}
			bindlessDescriptorFlags.push_back(0); //yes, we really need the 0.
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfo extendedLayoutInfo = VulkanAPI::DescriptorSetLayoutBindingFlagsCreateInfo(static_cast<uint32_t>(layoutBindings.size()), bindlessDescriptorFlags.data());
		if (isBindless)
			descriptorLayoutInfo.pNext = &extendedLayoutInfo;

		LUCY_VK_ASSERT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &m_DescriptorSetLayout));

		/*
		* if any of the bindings are bindless.
		* we are assuming that the set is being used ONLY for bindless descriptors
		* which means, we can't mix a binding which DOESN'T use bindless with a binding that USES bindless
		*/
		std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, m_DescriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo = VulkanAPI::DescriptorSetAllocateInfo(maxFramesInFlight, layouts.data(), descriptorPool->GetVulkanHandle());

		m_DescriptorSets.resize(maxFramesInFlight);
		LUCY_VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data()));

		RTInitializeBufferDescriptors();
	}

	void VulkanDescriptorSet::RTInitializeBufferDescriptors() {
		LUCY_PROFILE_NEW_EVENT("VulkanDescriptorSet::RTInitializeBufferDescriptors");
		
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		LUCY_ASSERT(!m_DescriptorSets.empty());

		const uint32_t maxFramesInFlight = m_CreateInfo.Count;

		for (uint32_t frameIndex = 0; frameIndex < maxFramesInFlight; frameIndex++)
			RTWriteBufferDescriptors(frameIndex);
	}

	void VulkanDescriptorSet::RTWriteBufferDescriptors(uint32_t frameIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanDescriptorSet::RTWriteBufferDescriptors");

		LUCY_ASSERT(Renderer::IsOnRenderThread());
		LUCY_ASSERT(!m_DescriptorSets.empty());

		VkDevice device = m_VulkanDevice->GetLogicalDevice();

		for (RenderDeviceResourceHandle bufferHandle : GetAllUniformBufferHandles() | std::views::values) {
			const auto& uniformBuffer = m_VulkanDevice->AccessResource<VulkanUniformBuffer>(bufferHandle);
			if (!uniformBuffer)
				continue;

			DescriptorType descriptorType = uniformBuffer->GetDescriptorType();
			uint32_t arraySize = uniformBuffer->GetArraySize();

			if (descriptorType.Shape != DescriptorBaseShape::ConstantBuffer)
				continue;

			uniformBuffer->RTLoadToDevice();

			VkDescriptorBufferInfo bufferInfo = VulkanAPI::DescriptorBufferInfo(uniformBuffer->GetVulkanBufferHandle(frameIndex), 0, VK_WHOLE_SIZE);
			VkWriteDescriptorSet setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[frameIndex], 0, uniformBuffer->GetBinding(), arraySize == 0 ? 1 : arraySize,
				(VkDescriptorType)ConvertDescriptorType(descriptorType), &bufferInfo);

			vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
			uniformBuffer->Clear();
		}

		for (RenderDeviceResourceHandle bufferHandle : GetAllSharedStorageBufferHandles() | std::views::values) {
			const auto& ssbo = m_VulkanDevice->AccessResource<VulkanSharedStorageBuffer>(bufferHandle);
			if (!ssbo)
				continue;

			DescriptorType descriptorType = ssbo->GetDescriptorType();
			uint32_t arraySize = ssbo->GetArraySize();

			if (descriptorType.Shape != DescriptorBaseShape::SharedStorageBuffer && descriptorType.Shape != DescriptorBaseShape::RWSharedStorageBuffer)
				continue;

			ssbo->RTLoadToDevice();

			VkDescriptorBufferInfo bufferInfo = VulkanAPI::DescriptorBufferInfo(ssbo->GetVulkanBufferHandle(frameIndex), 0, VK_WHOLE_SIZE);
			VkWriteDescriptorSet setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[frameIndex], 0, ssbo->GetBinding(), arraySize == 0 ? 1 : arraySize,
				(VkDescriptorType)ConvertDescriptorType(descriptorType), &bufferInfo);

			vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
			ssbo->Clear();
		}
	}

	void VulkanDescriptorSet::RTUpdate() {
		LUCY_PROFILE_NEW_EVENT("VulkanDescriptorSet::RTUpdate");

		LUCY_ASSERT(Renderer::IsOnRenderThread());
		LUCY_ASSERT(!m_DescriptorSets.empty());

		for (RenderDeviceResourceHandle bufferHandle : GetAllUniformBufferHandles() | std::views::values) {
			const auto& uniformBuffer = m_VulkanDevice->AccessResource<VulkanUniformBuffer>(bufferHandle);
			if (!uniformBuffer)
				continue;

			uniformBuffer->RTLoadToDevice();
			uniformBuffer->Clear();
		}

		for (RenderDeviceResourceHandle bufferHandle : GetAllSharedStorageBufferHandles() | std::views::values) {
			const auto& storageBuffer = m_VulkanDevice->AccessResource<VulkanSharedStorageBuffer>(bufferHandle);
			if (!storageBuffer)
				continue;

			storageBuffer->RTLoadToDevice();
			storageBuffer->Clear();
		}

		RTUpdateImageSamplerDescriptors();
	}

	void VulkanDescriptorSet::RTUpdateImageSamplerDescriptors() {
		LUCY_PROFILE_NEW_EVENT("VulkanDescriptorSet::RTUpdateImageSamplerDescriptors");
		
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		if (m_ImageSamplerBindingInfos.empty())
			return;

		VkDevice device = m_VulkanDevice->GetLogicalDevice();

		for (VulkanImageSamplerBindingInfo& bindingInfo : m_ImageSamplerBindingInfos | std::views::values) {
			auto& imageInfos = bindingInfo.ImageInfos;

			if (imageInfos.empty())
				continue;

			VkWriteDescriptorSet setWrite{};
			//if the descriptor set needs to be updated per frame (aka if the descriptor set is non-global)
			if (m_CreateInfo.Count == Renderer::GetMaxFramesInFlight()) {
				setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[frameIndex], 0, bindingInfo.Binding, static_cast<uint32_t>(imageInfos.size()),
					static_cast<VkDescriptorType>(ConvertDescriptorType(bindingInfo.DescriptorType)), nullptr, imageInfos.data());
			} else {
				setWrite = VulkanAPI::WriteDescriptorSet(m_DescriptorSets[0], 0, bindingInfo.Binding, static_cast<uint32_t>(imageInfos.size()),
					static_cast<VkDescriptorType>(ConvertDescriptorType(bindingInfo.DescriptorType)), nullptr, imageInfos.data());
			}

			vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);

			imageInfos.clear();
		}
	}

	VulkanImageSamplerBindingInfo* VulkanDescriptorSet::GetVulkanImageSampler(const std::string& imageBufferName) {
		if (!m_ImageSamplerBindingInfos.contains(imageBufferName))
			return nullptr;
		return &m_ImageSamplerBindingInfos.at(imageBufferName);
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