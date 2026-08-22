#pragma once

#include "DescriptorSet.h"

#include "Renderer/Image/Image.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"
#include "Renderer/Pipeline/VulkanImageSamplerBindingInfo.h"

namespace Lucy {

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
		void RTBake(const Ref<VulkanDescriptorPool>& descriptorPool, RenderDevice* device);
		void RTUpdate(RenderDevice* device) final override;
		void RTUpdateImageSamplerDescriptors(RenderDevice* device, const std::string& imageBufferName, const RenderDeviceTextureHandle& handle);

		VulkanImageSamplerBindingInfo* GetVulkanImageSampler(const std::string& imageBufferName);

		inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	private:
		void RTInitializeBufferDescriptors(RenderDevice* device);
		void RTWriteBufferDescriptors(uint32_t frameIndex, RenderDevice* device);

		void RTCreate(const Ref<VulkanRenderDevice>& vulkanDevice);
		void RTDestroyResource(RenderDevice* device) final override;

		std::unordered_map<std::string, VulkanImageSamplerBindingInfo> m_ImageSamplerBindingInfos;

		std::vector<VkDescriptorSet> m_DescriptorSets;
		VkDescriptorSetLayout m_DescriptorSetLayout;
	};
}

