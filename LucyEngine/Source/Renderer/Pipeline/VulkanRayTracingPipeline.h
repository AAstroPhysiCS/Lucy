#pragma once

#include "RayTracingPipeline.h"

namespace Lucy {

	class VulkanRenderDevice;

	class VulkanAccelerationStructure final : public AccelerationStructure {
	public:
		VulkanAccelerationStructure(const BLAccelerationStructureCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		VulkanAccelerationStructure(const TLAccelerationStructureCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanAccelerationStructure() = default;

		VulkanAccelerationStructure(const VulkanAccelerationStructure&) = delete;
		VulkanAccelerationStructure& operator=(const VulkanAccelerationStructure&) = delete;
		VulkanAccelerationStructure(VulkanAccelerationStructure&&) = delete;
		VulkanAccelerationStructure& operator=(VulkanAccelerationStructure&&) = delete;

		[[nodiscard]] VkAccelerationStructureKHR GetVulkanHandle() const { return m_AccelerationStructure; }
		[[nodiscard]] RenderDeviceBufferReference GetDeviceAddress() const final override { return m_DeviceAddress; }
		
		void RTUpdate(RenderDevice* device, const TLAccelerationStructureCreateInfo& createInfo) final override;
	private:
		void RTCreateBottomLevel(const Ref<VulkanRenderDevice>& device);
		void RTCreateTopLevel(const Ref<VulkanRenderDevice>& device);
		void RTDestroyResource(RenderDevice* device) final override;

		static std::vector<VkAccelerationStructureInstanceKHR> CreateVulkanInstances(const TLAccelerationStructureCreateInfo& createInfo);
	private:
		VkAccelerationStructureKHR m_AccelerationStructure = VK_NULL_HANDLE;
		RenderDeviceBufferReference m_DeviceAddress{};

		RenderDeviceResourceHandle m_StorageBufferHandle{};
		RenderDeviceResourceHandle m_ScratchBufferHandle{};
		RenderDeviceResourceHandle m_InstanceBufferHandle{};
	};

	class VulkanRenderDevice;

	class VulkanRayTracingPipeline final : public RayTracingPipeline {
	public:
		VulkanRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanRayTracingPipeline() = default;

		VulkanRayTracingPipeline(const VulkanRayTracingPipeline&) = delete;
		VulkanRayTracingPipeline& operator=(const VulkanRayTracingPipeline&) = delete;
		VulkanRayTracingPipeline(VulkanRayTracingPipeline&&) = delete;
		VulkanRayTracingPipeline& operator=(VulkanRayTracingPipeline&&) = delete;

		inline VkPipeline GetVulkanHandle() const { return m_PipelineHandle; }
		inline VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayoutHandle; }

		[[nodiscard]] const std::vector<RenderDeviceResourceHandle>& GetDescriptorSetHandles() const { return m_DescriptorSetHandles; }

		void RTRecreate(Ref<Shader> shader) final override;
		void RTTrace(void* commandBufferHandle, uint32_t width, uint32_t height, uint32_t depth) final override;
		void RTBind(void* commandBufferHandle) final override;
		void RTUpdateAccelerationStructure(RenderDevice* device, const std::string& name, const Ref<AccelerationStructure>& accelerationStructure);
	private:
		void Create(const Ref<VulkanRenderDevice>& vulkanDevice);
		void RTDestroyResource(RenderDevice* device) final override;

		void CreateShaderBindingTable(const Ref<VulkanRenderDevice>& vulkanDevice);

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		uint32_t m_ShaderGroupCount = 0;

		RenderDeviceResourceHandle m_ShaderBindingTableHandle{};

		VkStridedDeviceAddressRegionKHR m_RayGenRegion{};
		VkStridedDeviceAddressRegionKHR m_MissRegion{};
		VkStridedDeviceAddressRegionKHR m_HitRegion{};
		VkStridedDeviceAddressRegionKHR m_CallableRegion{};

		std::vector<RenderDeviceResourceHandle> m_DescriptorSetHandles;
		std::vector<VkDescriptorSetLayout> m_EmptyDescriptorSetLayouts;
	};
}
