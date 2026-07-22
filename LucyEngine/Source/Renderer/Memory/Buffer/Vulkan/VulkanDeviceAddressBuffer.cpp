#include "lypch.h"
#include "VulkanDeviceAddressBuffer.h"

#include "Renderer/Device/VulkanRenderDevice.h"
#include "Renderer/Context/VulkanContext.h"

namespace Lucy {

	VulkanDeviceAddressBuffer::VulkanDeviceAddressBuffer(RenderDeviceSize size, const Ref<VulkanRenderDevice>& device)
		: m_Size(size), m_Device(device) {
		RTCreate();
	}

	void VulkanDeviceAddressBuffer::RTCreate() {
        LUCY_ASSERT(m_Size > 0, "Cannot create zero-sized VulkanDeviceAddressBuffer!");

        auto result = m_Device->GetAllocator().CreateVulkanBufferVma(VulkanBufferUsage::CPUToGPU, m_Size, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, true, m_Buffer, m_Allocation);
		m_MappedData = result.pMappedData;

        QueryDeviceAddress();

        LUCY_ASSERT(m_DeviceAddress != 0, "Failed to query buffer device address!");
#ifdef LUCY_DEBUG
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Buffer);
        nameInfo.pObjectName = "VulkanDeviceAddressBuffer";

        VulkanExternalFuncLinkage::vkSetDebugUtilsObjectNameEXT(m_Device->GetLogicalDevice(), &nameInfo);
#endif
	}

	void VulkanDeviceAddressBuffer::RTLoadToDevice(const void* data, RenderDeviceSize size, RenderDeviceSize offset) {
        LUCY_ASSERT(data != nullptr, "Cannot upload null data to VulkanDeviceAddressBuffer!");
        LUCY_ASSERT(offset + size <= m_Size, "VulkanDeviceAddressBuffer upload out of bounds!");
        LUCY_ASSERT(m_MappedData != nullptr, "VulkanDeviceAddressBuffer is not mapped!");

		auto& allocator = m_Device->GetAllocator();
        memcpy(static_cast<uint8_t*>(m_MappedData) + offset, data, size);
        // Safe for coherent and non-coherent memory.
        allocator.Flush(m_Allocation, offset, size);

        m_SizeAllocated = std::max(m_SizeAllocated, offset + size);
	}

    void VulkanDeviceAddressBuffer::RTDestroyResource() {
        if (m_Buffer == VK_NULL_HANDLE)
            return;

        m_Device->GetAllocator().DestroyBuffer(m_Buffer, m_Allocation);

        m_Buffer = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
        m_MappedData = nullptr;
        m_DeviceAddress = 0;
        m_SizeAllocated = 0;
	}

	void VulkanDeviceAddressBuffer::QueryDeviceAddress() {
        VkBufferDeviceAddressInfo addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.buffer = m_Buffer;

        m_DeviceAddress = vkGetBufferDeviceAddress(m_Device->GetLogicalDevice(), &addressInfo);
	}
}