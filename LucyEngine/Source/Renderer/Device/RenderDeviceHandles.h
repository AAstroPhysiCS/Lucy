#pragma once

#include "Utilities/GenerationalPool.h"

namespace Lucy {

    struct RenderDeviceResourceTag;
    using RenderDeviceResourceHandle = GenerationalHandle<RenderDeviceResourceTag>;

    struct RenderDeviceObjectTag;
    using RenderDeviceObjectHandle = GenerationalHandle<RenderDeviceObjectTag, uint64_t>;

    //struct RenderDeviceMaterialTag;
    //using RenderDeviceMaterialHandle = GenerationalHandle<RenderDeviceMaterialTag, uint64_t>;
}