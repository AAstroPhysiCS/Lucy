#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

    class RenderDevice;

    enum class SemaphoreType : uint8_t {
        Timeline,
		Binary
    };

	class Semaphore {
    public:
        Semaphore() = default;
        Semaphore(SemaphoreType type, const Ref<RenderDevice>& device);
        virtual ~Semaphore() = default;

        Semaphore(const Semaphore& other) = default;
        Semaphore(Semaphore&& other) noexcept = default;
        Semaphore& operator=(const Semaphore& other) = default;
        Semaphore& operator=(Semaphore&& other) noexcept = default;

        virtual void* GetHandle() const = 0;
        virtual void Destroy() = 0;

        virtual void Wait(uint64_t value) = 0;
        virtual bool IsComplete(uint64_t value) const = 0;
    protected:
		inline Ref<RenderDevice> GetRenderDevice() const { return m_RenderDevice; }
		inline SemaphoreType GetType() const { return m_Type; }
    private:
		SemaphoreType m_Type = SemaphoreType::Binary;
        Ref<RenderDevice> m_RenderDevice = nullptr;
	};

    /* 
        * in D3D12, fences are basically timeline semaphores.
        * It does not have a CPU-CPU sync item like VkFence, so i made the decision to implement
        * this class D3D12-like for a better abstraction.
    */
    
    class VulkanSemaphore final : public Semaphore {
    public:
        VulkanSemaphore() = default;
        VulkanSemaphore(SemaphoreType type, Ref<RenderDevice> device);
        virtual ~VulkanSemaphore() = default;

        void* GetHandle() const final override;
        void Destroy() final override;

        void Wait(uint64_t value) final override;
        bool IsComplete(uint64_t value) const final override;

        inline VkSemaphore GetSemaphore() const { return m_Handle; }
    private:
        uint64_t GetCompletedValue() const;

        VkSemaphore m_Handle = VK_NULL_HANDLE;
    };

    static inline constexpr void DefineMasksByLayout(VkImageLayout oldLayout, VkImageLayout newLayout,
        VkAccessFlags2& srcAccessMask, VkAccessFlags2& dstAccessMask,
        VkPipelineStageFlags2& sourceStage, VkPipelineStageFlags2& destStage) {

        switch (oldLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                srcAccessMask = VK_ACCESS_2_NONE;
                sourceStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                srcAccessMask = VK_ACCESS_2_NONE;
                sourceStage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
                break;
            case VK_IMAGE_LAYOUT_GENERAL: // primarily for compute
                srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                break;
            case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
                srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
                sourceStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
                break;
            default:
                LUCY_ASSERT(false);
        }

        switch (newLayout) {
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                destStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                destStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
                destStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                break;
            case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
                dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR;
                destStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                destStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                destStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                dstAccessMask = VK_ACCESS_2_NONE;
                destStage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                destStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
                break;
            case VK_IMAGE_LAYOUT_GENERAL:
                dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                destStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                break;
            default:
                LUCY_ASSERT(false);
        }
    }
}
