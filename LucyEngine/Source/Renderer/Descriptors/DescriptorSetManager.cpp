#include "lypch.h"
#include "DescriptorSetManager.h"

#include "Renderer/Descriptors/VulkanDescriptorSet.h"
#include "Renderer/Device/VulkanRenderDevice.h"
#include "Renderer/Image/ImageSampler.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/RenderDeviceScene.h"

namespace Lucy {
	
	VulkanDescriptorSetManager::VulkanDescriptorSetManager(VulkanRenderDevice* device) 
		: m_RenderDevice(device) {
		const uint32_t frames = Renderer::GetMaxFramesInFlight();
		
		{
			constexpr uint32_t maxBindlessSampledImages = 24576;
			constexpr uint32_t maxBindlessStorageImages = 24576 / 2;
			constexpr uint32_t maxBindlessSamplers = 4096;

			constexpr uint32_t sampledImageBindlessBindings = 6;
			constexpr uint32_t storageImageBindlessBindings = 4;
			constexpr uint32_t samplerBindlessBindings = 1;

			const std::vector<VkDescriptorPoolSize> poolSizes = {
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frames * 4 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frames * 4 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sampledImageBindlessBindings * maxBindlessSampledImages },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storageImageBindlessBindings * maxBindlessStorageImages },
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 16 },
				{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, frames * 16 }
			};

			VulkanDescriptorPoolCreateInfo poolCreateInfo;
			poolCreateInfo.PoolSizesVector = poolSizes;
			poolCreateInfo.MaxSet = 2 * frames;
			poolCreateInfo.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			poolCreateInfo.LogicalDevice = m_RenderDevice->GetLogicalDevice();

			m_GlobalDescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);
		}

		{
			constexpr uint32_t maxShaders = 32;
			constexpr uint32_t maxPerShaderSets = 2;
			constexpr uint32_t maxLogicalDescriptorSets = maxShaders * maxPerShaderSets;

			const std::vector<VkDescriptorPoolSize> poolSizes = {
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frames * 16 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frames * 16 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frames * 16 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, frames * 16 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, frames * 16 },
				{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, frames * 16 }
			};

			VulkanDescriptorPoolCreateInfo poolCreateInfo;
			poolCreateInfo.PoolSizesVector = poolSizes;
			poolCreateInfo.MaxSet = maxLogicalDescriptorSets * frames;
			poolCreateInfo.PoolFlags = 0;
			poolCreateInfo.LogicalDevice = m_RenderDevice->GetLogicalDevice();

			m_PerShaderDescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);
		}
	}

	void VulkanDescriptorSetManager::RegisterShaderBindings(const Ref<Shader>& shader) {
		const auto& reflectUniformBlockMaps = shader->GetShaderUniformBlockMap();
		const auto& name = shader->GetName();

		const auto CreateDescriptorSet = [&](auto& descriptorSets, const auto& descriptorPool, const auto& createInfo) {
			RenderDeviceResourceHandle descriptorSetHandle = m_RenderDevice->CreateDescriptorSet(createInfo);
			const auto& descriptorSet = m_RenderDevice->AccessResource<VulkanDescriptorSet>(descriptorSetHandle);
			descriptorSet->Bake(descriptorPool, m_RenderDevice);
			descriptorSets.try_emplace(createInfo.SetIndex, descriptorSetHandle);
		};

		for (const auto& [set, info] : reflectUniformBlockMaps) {
			DescriptorSetCreateInfo createInfo{
				.SetIndex = set,
				.ShaderVariables = info,
			};

			if (!m_GlobalDescriptorSets.contains(set) && (set == TEXTURE_BINDLESS_TABLE_SET_INDEX || set == GLOBAL_PER_FRAME_SET_INDEX)) {
				// global descriptor sets are created from the first shader that references them
				// include all supported shader stages here, otherwise the shared layout may be
				// created with incomplete stage flags and later cause Vulkan validation errors
				for (auto& variable : createInfo.ShaderVariables)
					variable.StageFlag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
				createInfo.Count = 1; //we only need one since its global bindless descriptor set.
				CreateDescriptorSet(m_GlobalDescriptorSets, m_GlobalDescriptorPool, createInfo);
				continue;
			}

			if (set == TEXTURE_BINDLESS_TABLE_SET_INDEX || set == GLOBAL_PER_FRAME_SET_INDEX)
				continue;

			for (auto& variable : createInfo.ShaderVariables)
				variable.StageFlag |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			CreateDescriptorSet(m_DescriptorSetsPerShader, m_PerShaderDescriptorPool, createInfo);
		}

		LUCY_INFO("Parsed shader {0} and added descriptor sets.", name);
	}

	void VulkanDescriptorSetManager::RTDestroy() {
		for (auto& [_, handle] : m_DescriptorSetsPerShader)
			Renderer::EnqueueResourceDestroy(handle);
		for (auto& [_, handle] : m_GlobalDescriptorSets)
			Renderer::EnqueueResourceDestroy(handle);
		
		m_DescriptorSetsPerShader.clear();
		m_GlobalDescriptorSets.clear();

		m_GlobalDescriptorPool->RTDestroyResource();
		m_GlobalDescriptorPool = nullptr;

		m_PerShaderDescriptorPool->RTDestroyResource();
		m_PerShaderDescriptorPool = nullptr;
	}

	std::vector<RenderDeviceResourceHandle> VulkanDescriptorSetManager::GetDescriptorSetHandles(const Ref<Shader>& shader) {
		std::vector<RenderDeviceResourceHandle> result;
		const auto& reflectUniformBlockMaps = shader->GetShaderUniformBlockMap();

		for (const auto& [set, info] : reflectUniformBlockMaps) {
			if (m_GlobalDescriptorSets.contains(set))
				result.push_back(m_GlobalDescriptorSets.at(set));
			if (m_DescriptorSetsPerShader.contains(set))
				result.push_back(m_DescriptorSetsPerShader.at(set));
		}

		return result;
	}
}