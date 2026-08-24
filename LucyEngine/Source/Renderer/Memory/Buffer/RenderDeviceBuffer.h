#pragma once

#include "Renderer/Device/RenderDeviceResource.h"
#include "Renderer/Memory/VulkanAllocator.h" //TODO: Change this

namespace Lucy {

    class RenderDevice;

    using RenderDeviceBufferReference = uint64_t;
    using RenderDeviceSize = uint64_t;

    enum class BufferUsage : uint32_t {
        None = 0,

        TransferSource = 1 << 0,
        TransferDestination = 1 << 1,

        Vertex = 1 << 2,
        Index = 1 << 3,

        Uniform = 1 << 4,
        Storage = 1 << 5,

        Indirect = 1 << 6,
        ShaderDeviceAddress = 1 << 7,

        AccelerationStructureBuildInput = 1 << 8,
        AccelerationStructureStorage = 1 << 9,
        ShaderBindingTable = 1 << 10
    };

    //fucking hate enum classes
    [[nodiscard]] constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) {
        return static_cast<BufferUsage>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    //fucking hate enum classes
    [[nodiscard]] constexpr BufferUsage operator&(BufferUsage lhs, BufferUsage rhs) {
        return static_cast<BufferUsage>(std::to_underlying(lhs) & std::to_underlying(rhs));
    }

    struct RenderDeviceBufferCreateInfo {
        std::string DebugName = "RenderDeviceBuffer";
        RenderDeviceSize Size = 0;
        BufferUsage Usage = BufferUsage::None;
        MemoryUsage MemoryUsage = MemoryUsage::GPUOnly;
    };

    class RenderDeviceBuffer : public RenderDeviceResource {
    public:
        virtual ~RenderDeviceBuffer() = default;

        RenderDeviceBuffer(const RenderDeviceBuffer&) = delete;
        RenderDeviceBuffer& operator=(const RenderDeviceBuffer&) = delete;
        RenderDeviceBuffer(RenderDeviceBuffer&&) = delete;
        RenderDeviceBuffer& operator=(RenderDeviceBuffer&&) = delete;

        virtual void RTLoadToDevice(RenderDevice* device, const void* data, RenderDeviceSize size, RenderDeviceSize offset = 0) = 0;
        virtual void* GetMappedData() const = 0;

        virtual RenderDeviceSize GetSize() const = 0;
        virtual RenderDeviceSize GetAllocatedSize() const = 0;
        virtual RenderDeviceBufferReference GetDeviceAddress() const = 0;

        const RenderDeviceBufferCreateInfo& GetCreateInfo() const { return m_CreateInfo; }
    protected:
        RenderDeviceBuffer(const RenderDeviceBufferCreateInfo& createInfo)
            : RenderDeviceResource(createInfo.DebugName), m_CreateInfo(createInfo) {
        }
    private:
        RenderDeviceBufferCreateInfo m_CreateInfo;
    };

    [[nodiscard]] inline constexpr VkBufferUsageFlags ToVulkanBufferUsage(BufferUsage usage) {
        VkBufferUsageFlags result = 0;
        
        const auto HasBufferUsage = [](auto usage, auto flag) { return (usage & flag) == flag; };

        if (HasBufferUsage(usage, BufferUsage::TransferSource))
            result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if (HasBufferUsage(usage, BufferUsage::TransferDestination))
            result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (HasBufferUsage(usage, BufferUsage::Vertex))
            result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        if (HasBufferUsage(usage, BufferUsage::Index))
            result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if (HasBufferUsage(usage, BufferUsage::Uniform))
            result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        if (HasBufferUsage(usage, BufferUsage::Storage))
            result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        if (HasBufferUsage(usage, BufferUsage::Indirect))
            result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

        if (HasBufferUsage(usage, BufferUsage::ShaderDeviceAddress))
            result |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        
        if (HasBufferUsage(usage, BufferUsage::AccelerationStructureBuildInput))
            result |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        if (HasBufferUsage(usage, BufferUsage::AccelerationStructureStorage))
            result |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
        
        if (HasBufferUsage(usage, BufferUsage::ShaderBindingTable))
            result |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;

        return result;
    }
}