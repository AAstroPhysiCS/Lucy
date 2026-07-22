#pragma once

#include "DescriptorSet.h"

#include "Renderer/Image/Image.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"
#include "Renderer/Pipeline/VulkanImageSamplerBindingInfo.h"

namespace Lucy {

	struct VulkanDescriptorSetCreateInfo {
		VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
		Ref<VulkanDescriptorPool> Pool = nullptr;
	};

	struct VulkanDescriptorSetBindInfo {
		VkCommandBuffer CommandBuffer;
		VkPipelineBindPoint PipelineBindPoint;
		VkPipelineLayout PipelineLayout;
	};

	class VulkanDescriptorSet : public DescriptorSet {
	public:
		VulkanDescriptorSet(const DescriptorSetCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanDescriptorSet() = default;

		VulkanDescriptorSet(const VulkanDescriptorSet&) = delete;
		VulkanDescriptorSet& operator=(const VulkanDescriptorSet&) = delete;
		VulkanDescriptorSet(VulkanDescriptorSet&&) = delete;
		VulkanDescriptorSet& operator=(VulkanDescriptorSet&&) = delete;

		void RTBind(const VulkanDescriptorSetBindInfo& bindInfo);
		void RTBake(const Ref<VulkanDescriptorPool>& descriptorPool);
		void RTUpdate() final override;
		
		VulkanImageSamplerBindingInfo* GetVulkanImageSampler(const std::string& imageBufferName);

		inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	private:
		void RTInitializeBufferDescriptors();
		void RTWriteBufferDescriptors(uint32_t frameIndex);
		void RTUpdateImageSamplerDescriptors();

		void RTCreate();
		void RTDestroyResource() final override;

		std::unordered_map<std::string, VulkanImageSamplerBindingInfo> m_ImageSamplerBindingInfos;

		std::vector<VkDescriptorSet> m_DescriptorSets;
		VkDescriptorSetLayout m_DescriptorSetLayout;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}

