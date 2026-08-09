#pragma once

#include "Renderer/Memory/Buffer/RenderDeviceBuffer.h"
#include "Renderer/Memory/VulkanAllocator.h"

namespace Lucy {

    class VulkanRenderDevice;

    class VulkanDeviceAddressBuffer final : public RenderDeviceBuffer {
    public:
        VulkanDeviceAddressBuffer(const RenderDeviceBufferCreateInfo& createInfo, const Ref<RenderDevice>& device);
        ~VulkanDeviceAddressBuffer() override = default;

        VulkanDeviceAddressBuffer(const VulkanDeviceAddressBuffer&) = delete;
        VulkanDeviceAddressBuffer& operator=(const VulkanDeviceAddressBuffer&) = delete;
        VulkanDeviceAddressBuffer(VulkanDeviceAddressBuffer&&) = delete;
        VulkanDeviceAddressBuffer& operator=(VulkanDeviceAddressBuffer&&) = delete;

        void RTLoadToDevice(RenderDevice* device, const void* data, RenderDeviceSize size, RenderDeviceSize offset = 0) final override;
        void RTDestroyResource(RenderDevice* device) final override;

        void* GetMappedData() const override { return m_MappedData; }

        RenderDeviceSize GetSize() const override { return GetCreateInfo().Size; }
        RenderDeviceSize GetAllocatedSize() const override { return m_SizeAllocated; }
        RenderDeviceBufferReference GetDeviceAddress() const override { return static_cast<RenderDeviceBufferReference>(m_DeviceAddress); }

        VkBuffer GetVulkanBufferHandle() const { return m_Buffer; }
        BufferUsage GetUsage() const { return GetCreateInfo().Usage; }
    private:
        void RTCreate(const Ref<RenderDevice>& device);
        void QueryDeviceAddress(const Ref<VulkanRenderDevice>& vulkanDevice);
    private:
        RenderDeviceSize m_SizeAllocated = 0;

        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        void* m_MappedData = nullptr;
        VkDeviceAddress m_DeviceAddress = 0;
    };
}
