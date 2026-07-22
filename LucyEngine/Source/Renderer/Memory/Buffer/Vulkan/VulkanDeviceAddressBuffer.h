#pragma once

#include "Renderer/Memory/Buffer/RenderDeviceBuffer.h"
#include "Renderer/Memory/VulkanAllocator.h"

namespace Lucy {

    class VulkanRenderDevice;

    class VulkanDeviceAddressBuffer final : public RenderDeviceBuffer {
    public:
        VulkanDeviceAddressBuffer(RenderDeviceSize size, const Ref<VulkanRenderDevice>& device);
        ~VulkanDeviceAddressBuffer() override = default;

        VulkanDeviceAddressBuffer(const VulkanDeviceAddressBuffer&) = delete;
        VulkanDeviceAddressBuffer& operator=(const VulkanDeviceAddressBuffer&) = delete;
        VulkanDeviceAddressBuffer(VulkanDeviceAddressBuffer&&) = delete;
        VulkanDeviceAddressBuffer& operator=(VulkanDeviceAddressBuffer&&) = delete;

        void RTLoadToDevice(const void* data, RenderDeviceSize size, RenderDeviceSize offset = 0) override;
        void RTDestroyResource() override;

        void* GetMappedData() const override { return m_MappedData; }

        RenderDeviceSize GetSize() const override { return m_Size; }
        RenderDeviceSize GetAllocatedSize() const override { return m_SizeAllocated; }
        RenderDeviceBufferReference GetDeviceAddress() const override { return static_cast<RenderDeviceBufferReference>(m_DeviceAddress); }

        VkBuffer GetVulkanBufferHandle() const { return m_Buffer; }
    private:
        void RTCreate();
        void QueryDeviceAddress();
    private:
        RenderDeviceSize m_Size = 0;
        RenderDeviceSize m_SizeAllocated = 0;

        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        void* m_MappedData = nullptr;
        VkDeviceAddress m_DeviceAddress = 0;

        Ref<VulkanRenderDevice> m_Device = nullptr;
    };
}
