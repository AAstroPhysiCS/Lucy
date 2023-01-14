#include "lypch.h"
#include "ContextPipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "VulkanGraphicsPipeline.h"
#include "VulkanComputePipeline.h"

#include "VulkanContextDevice.h"

namespace Lucy {

	Ref<ContextPipeline> ContextPipeline::Create(const GraphicsPipelineCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanGraphicsPipeline>(createInfo);
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return nullptr;
	}

	Ref<ContextPipeline> ContextPipeline::Create(const ComputePipelineCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanComputePipeline>(createInfo);
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return nullptr;
	}

	ContextPipeline::ContextPipeline(ContextPipelineType type)
		: m_Type(type) {
	}

	VulkanPushConstant& ContextPipeline::GetPushConstants(const char* name) {
		for (VulkanPushConstant& pushConstant : m_PushConstants) {
			if (name == pushConstant.GetName()) {
				return pushConstant;
			}
		}
		LUCY_ASSERT(false, "Could not find a suitable Push Constant for the given name: {0}", name);
	}

	void VulkanContextPipelineUtils::ParseVulkanDescriptorSets(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, const Ref<Shader>& shader, 
															   Ref<VulkanDescriptorPool>& descriptorPool, std::vector<Ref<DescriptorSet>>& descriptorSets, 
															   std::vector<VulkanPushConstant>& pushConstants) {
		if (descriptorSetLayouts.size() != 0) //if the application has been resized
			return;

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		for (auto& [set, info] : shader->GetShaderUniformBlockMap()) {
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			std::vector<bool> isBindlessVector;

			for (auto& buffer : info) {
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
			* cant summarize these since these are stack allocated
			*
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

			VkDescriptorSetLayout descriptorSetLayout;
			LUCY_VK_ASSERT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayout));
			descriptorSetLayouts.push_back(descriptorSetLayout);

			DescriptorSetCreateInfo setCreateInfo;
			setCreateInfo.SetIndex = set;

			/*
			* if any of the bindings are bindless.
			* we are assuming that the set is being used ONLY for bindless descriptors
			* which means, we can't mix a binding which DOESN'T use bindless with a binding that USES bindless
			*/
			setCreateInfo.Bindless = std::any_of(isBindlessVector.begin(), isBindlessVector.end(), [](bool out) { return out; });

			auto setInternalInfo = Memory::CreateRef<VulkanDescriptorSetCreateInfo>();
			setInternalInfo->Layout = descriptorSetLayout;
			setInternalInfo->Pool = descriptorPool;

			setCreateInfo.InternalInfo = setInternalInfo;

			Ref<DescriptorSet> descriptorSet = DescriptorSet::Create(setCreateInfo);

			for (const auto& buffer : info) {
				switch (buffer.Type) {
					case DescriptorType::SSBODynamic:
					case DescriptorType::SSBO: {
						SharedStorageBufferCreateInfo createInfo;
						createInfo.Name = buffer.Name;
						createInfo.Binding = buffer.Binding;
						createInfo.Type = buffer.Type;
						createInfo.BufferSize = MAX_DYNAMICALLY_ALLOCATED_BUFFER_SIZE;
						createInfo.ArraySize = buffer.ArraySize;
						createInfo.ShaderMemberVariables = buffer.Members;

						descriptorSet->AddBuffer(SharedStorageBuffer::Create(createInfo));
						break;
					}
					default: { //all the other descriptor types are uniforms
						UniformBufferCreateInfo createInfo;
						createInfo.Name = buffer.Name;
						createInfo.Binding = buffer.Binding;
						createInfo.Type = buffer.Type;
						createInfo.BufferSize = buffer.BufferSize;
						createInfo.ArraySize = buffer.ArraySize;
						createInfo.ShaderMemberVariables = buffer.Members;

						descriptorSet->AddBuffer(UniformBuffer::Create(createInfo));
						break;
					}
				}
			}

			descriptorSets.push_back(descriptorSet);
		}

		auto& pushConstantMap = shader->GetPushConstants();
		for (auto& pc : pushConstantMap)
			pushConstants.push_back(VulkanPushConstant(pc.Name, pc.BufferSize, 0, pc.StageFlag));
	}
}