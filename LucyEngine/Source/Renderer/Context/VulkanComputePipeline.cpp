#include "lypch.h"
#include "VulkanComputePipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/Shader/VulkanComputeShader.h"
#include "Renderer/Context/VulkanContextDevice.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"

namespace Lucy {

	VulkanComputePipeline::VulkanComputePipeline(const Ref<ComputeShader>& computeShader)
		: ComputePipeline(computeShader) {
		Renderer::EnqueueToRenderThread([=]() {
			Create();
		});
	}

	void VulkanComputePipeline::Create() {
		const std::vector<VkDescriptorPoolSize> poolSizes = {
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 }
		};

		VulkanDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.PoolSizesVector = poolSizes;
		poolCreateInfo.MaxSet = 100;
		poolCreateInfo.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		m_DescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);

		ParseDescriptorSets();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_DescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();

		std::vector<VkPushConstantRange> pushConstantRanges;
		for (VulkanPushConstant& pc : m_PushConstants)
			pushConstantRanges.push_back(pc.GetHandle());

		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = m_PipelineLayoutHandle;
		pipelineInfo.stage = m_ComputeShader.As<VulkanComputeShader>()->GetShaderStageInfo();

		LUCY_VK_ASSERT(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineHandle));
	}

	void VulkanComputePipeline::Bind(void* commandBufferHandle) {
		vkCmdBindPipeline((VkCommandBuffer)commandBufferHandle, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineHandle);
	}

	void VulkanComputePipeline::Dispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
		vkCmdDispatch((VkCommandBuffer)commandBufferHandle, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanComputePipeline::Destroy() {
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		for (const auto& set : m_DescriptorSets)
			set->Destroy();

		for (auto& descriptorSetLayout : m_DescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		m_DescriptorPool->Destroy();
		vkDestroyPipelineLayout(device, m_PipelineLayoutHandle, nullptr);
		vkDestroyPipeline(device, m_PipelineHandle, nullptr);
	}

	//TODO: Abstract this with VulkanGraphicsPipeline
	void VulkanComputePipeline::ParseDescriptorSets() {
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		for (auto& [set, info] : m_ComputeShader->GetShaderUniformBlockMap()) {
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			std::vector<bool> isBindlessVector;

			for (auto& buffer : info) {
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = buffer.Binding;
				binding.descriptorCount = buffer.ArraySize == 0 ? 1 : buffer.ArraySize;

				isBindlessVector.push_back(buffer.DynamicallyAllocated); //the set is bindless if true

				if (buffer.DynamicallyAllocated) {
					buffer.ArraySize = MAX_DYNAMIC_DESCRIPTOR_COUNT;
					binding.descriptorCount = buffer.ArraySize;
				}

				binding.descriptorType = (VkDescriptorType)ConvertDescriptorType(buffer.Type);
				binding.stageFlags = buffer.StageFlag;
				binding.pImmutableSamplers = nullptr;

				layoutBindings.push_back(binding);
			}

			VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
			descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayoutInfo.bindingCount = layoutBindings.size();
			descriptorLayoutInfo.pBindings = layoutBindings.data();
			descriptorLayoutInfo.pNext = nullptr;

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

			VkDescriptorSetLayoutBindingFlagsCreateInfo extendedLayoutInfo{};
			extendedLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			extendedLayoutInfo.bindingCount = layoutBindings.size();
			extendedLayoutInfo.pBindingFlags = bindlessDescriptorFlags.data();

			descriptorLayoutInfo.pNext = &extendedLayoutInfo;

			VkDescriptorSetLayout descriptorSetLayout;
			LUCY_VK_ASSERT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayout));
			m_DescriptorSetLayouts.push_back(descriptorSetLayout);

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
			setInternalInfo->Pool = m_DescriptorPool;

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

			m_DescriptorSets.push_back(descriptorSet);
		}

		auto& pushConstantMap = m_ComputeShader->GetPushConstants();
		for (auto& pc : pushConstantMap)
			m_PushConstants.push_back(VulkanPushConstant(pc.Name, pc.BufferSize, 0, pc.StageFlag));
	}
}