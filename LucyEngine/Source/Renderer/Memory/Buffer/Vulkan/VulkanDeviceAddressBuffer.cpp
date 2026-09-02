#include "lypch.h"
#include "VulkanDeviceAddressBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"
#include "Renderer/Context/VulkanContext.h"

namespace Lucy {

    VulkanDeviceAddressBuffer::VulkanDeviceAddressBuffer(const RenderDeviceBufferCreateInfo& createInfo, const Ref<RenderDevice>& device)
        : RenderDeviceBuffer(createInfo) {
        RTCreate(device);
    }

	void VulkanDeviceAddressBuffer::RTCreate(const Ref<RenderDevice>& device) {
        LUCY_ASSERT(GetSize() > 0, "Cannot create zero-sized VulkanDeviceAddressBuffer!");
        const auto& vulkanDevice = device->As<VulkanRenderDevice>();

        VkBufferUsageFlags vulkanUsage = ToVulkanBufferUsage(GetUsage());
        vulkanUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        auto memoryUsage = GetCreateInfo().MemoryUsage;
		auto sharingMode = GetCreateInfo().ShareAmongQueues ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

        if (memoryUsage == MemoryUsage::GPUOnly) {
            auto result = vulkanDevice->GetAllocator().CreateVulkanBufferVma(memoryUsage, GetSize(), vulkanUsage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, false, m_Buffer, m_Allocation, sharingMode);
            m_MappedData = result.pMappedData;
        } else {
            auto result = vulkanDevice->GetAllocator().CreateVulkanBufferVma(memoryUsage, GetSize(), vulkanUsage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, true, m_Buffer, m_Allocation, sharingMode);
            m_MappedData = result.pMappedData;
        }

        QueryDeviceAddress(vulkanDevice);

        auto name = std::format("VulkanDeviceAddressBuffer: {0}", GetDebugName());

        LUCY_ASSERT(m_DeviceAddress != 0, "Failed to query buffer device address!");
#ifdef LUCY_DEBUG
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Buffer);
        nameInfo.pObjectName = name.c_str();

        VulkanExternalFuncLinkage::vkSetDebugUtilsObjectNameEXT(vulkanDevice->GetLogicalDevice(), &nameInfo);
#endif
	}

    void VulkanDeviceAddressBuffer::RTLoadToDevice(RenderDevice* device, const void* data, RenderDeviceSize size, RenderDeviceSize offset) {
        LUCY_ASSERT(data);
        LUCY_ASSERT(offset + size <= GetSize());

        auto* vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);
        vulkanDevice->GetUploadManager()->EnqueueUploadBuffer(m_Buffer, offset, data, size);

        m_SizeAllocated = std::max(m_SizeAllocated, offset + size);
	}

    void VulkanDeviceAddressBuffer::RTDestroyResource(RenderDevice* device) {
        if (m_Buffer == VK_NULL_HANDLE)
            return;
        const auto& vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);
        vulkanDevice->GetAllocator().DestroyBuffer(m_Buffer, m_Allocation);

        m_Buffer = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
        m_MappedData = nullptr;
        m_DeviceAddress = 0;
        m_SizeAllocated = 0;
	}

	void VulkanDeviceAddressBuffer::QueryDeviceAddress(const Ref<VulkanRenderDevice>& vulkanDevice) {
        VkBufferDeviceAddressInfo addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.buffer = m_Buffer;

        m_DeviceAddress = vkGetBufferDeviceAddress(vulkanDevice->GetLogicalDevice(), &addressInfo);
	}
}