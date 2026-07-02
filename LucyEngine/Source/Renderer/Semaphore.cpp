#include "lypch.h"
#include "Semaphore.h"

#include "Device/VulkanRenderDevice.h"

namespace Lucy {

    Semaphore::Semaphore(SemaphoreType type, const Ref<RenderDevice>& device)
        : m_Type(type), m_RenderDevice(device) {
    }

    VulkanSemaphore::VulkanSemaphore(SemaphoreType type, Ref<RenderDevice> device)
        : Semaphore(type, device) {
        const auto& vulkanDevice = device->As<VulkanRenderDevice>();

        if (GetType() == SemaphoreType::Timeline) {
            VkSemaphoreTypeCreateInfo timelineInfo{};
            timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineInfo.initialValue = 0;

            VkSemaphoreCreateInfo createInfo = VulkanAPI::SemaphoreCreateInfo(0, &timelineInfo);
            LUCY_VK_ASSERT(vkCreateSemaphore(vulkanDevice->GetLogicalDevice(), &createInfo, nullptr, &m_Handle));
            return;
        }

        VkSemaphoreCreateInfo createInfo = VulkanAPI::SemaphoreCreateInfo(0, nullptr);
        LUCY_VK_ASSERT(vkCreateSemaphore(vulkanDevice->GetLogicalDevice(), &createInfo, nullptr, &m_Handle));
    }

    void* VulkanSemaphore::GetHandle() const {
        return m_Handle;
    }

    void VulkanSemaphore::Destroy() {
        const auto& vulkanDevice = GetRenderDevice()->As<VulkanRenderDevice>();
        vkDestroySemaphore(vulkanDevice->GetLogicalDevice(), m_Handle, nullptr);
    }

    void VulkanSemaphore::Wait(uint64_t value) {
        const auto& vulkanDevice = GetRenderDevice()->As<VulkanRenderDevice>();

        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_Handle;
        waitInfo.pValues = &value;

        LUCY_VK_ASSERT(vkWaitSemaphores(vulkanDevice->GetLogicalDevice(), &waitInfo, UINT64_MAX));
    }

    bool VulkanSemaphore::IsComplete(uint64_t value) const {
        return GetCompletedValue() >= value;
    }

    uint64_t VulkanSemaphore::GetCompletedValue() const {
        const auto& vulkanDevice = GetRenderDevice()->As<VulkanRenderDevice>();

        uint64_t value = 0;
        vkGetSemaphoreCounterValue(vulkanDevice->GetLogicalDevice(), m_Handle, &value);
        return value;
    }
}