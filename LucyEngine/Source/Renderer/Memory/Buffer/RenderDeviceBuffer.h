#pragma once

#include "Renderer/Device/RenderDeviceResource.h"

namespace Lucy {

    using RenderDeviceBufferReference = uint64_t;
    using RenderDeviceSize = uint64_t;

    class RenderDeviceBuffer : public RenderDeviceResource {
    public:
        virtual ~RenderDeviceBuffer() = default;

        RenderDeviceBuffer(const RenderDeviceBuffer&) = delete;
        RenderDeviceBuffer& operator=(const RenderDeviceBuffer&) = delete;
        RenderDeviceBuffer(RenderDeviceBuffer&&) = delete;
        RenderDeviceBuffer& operator=(RenderDeviceBuffer&&) = delete;

        virtual void RTLoadToDevice(const void* data, RenderDeviceSize size, RenderDeviceSize offset = 0) = 0;
        virtual void* GetMappedData() const = 0;

        virtual RenderDeviceSize GetSize() const = 0;
        virtual RenderDeviceSize GetAllocatedSize() const = 0;
        virtual RenderDeviceBufferReference GetDeviceAddress() const = 0;
    protected:
        RenderDeviceBuffer() 
            : RenderDeviceResource("RenderDeviceBuffer") {
        }
    };
}