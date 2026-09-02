#pragma once

#include "DescriptorSet.h"

#include "Renderer/Image/Image.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"
#include "Renderer/Pipeline/VulkanImageSamplerBindingInfo.h"

namespace Lucy {

	class AccelerationStructure;

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
		void Bake(const Ref<VulkanDescriptorPool>& descriptorPool, RenderDevice* device);
		
		void RTUpdate(RenderDevice* device) final override;
		void RTUpdateImageDescriptors(RenderDevice* device, const std::string& imageBufferName, const RenderDeviceTextureHandle& handle);
		void RTUpdateSamplerDescriptors(RenderDevice* device, const RenderDeviceResourceHandle& samplerHandle);
		void RTUpdateAccelerationStructure(RenderDevice* device, const std::string& name, const Ref<AccelerationStructure>& accelerationStructure);

		[[nodiscard]] bool HasAccelerationStructureBinding(const std::string& name) const { return m_AccelerationStructureBindings.contains(name); }

		VulkanImageSamplerBindingInfo* GetVulkanImageSampler(const std::string& imageBufferName);

		inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	private:
		void InitializeBufferDescriptors(RenderDevice* device);
		void WriteBufferDescriptors(uint32_t frameIndex, RenderDevice* device);

		void RTCreate(const Ref<VulkanRenderDevice>& vulkanDevice);
		void RTDestroyResource(RenderDevice* device) final override;

		std::unordered_map<std::string, VulkanImageSamplerBindingInfo> m_ImageSamplerBindingInfos;
		std::unordered_map<std::string, uint32_t> m_AccelerationStructureBindings;

		std::vector<VkDescriptorSet> m_DescriptorSets;
		VkDescriptorSetLayout m_DescriptorSetLayout;
	};
}

