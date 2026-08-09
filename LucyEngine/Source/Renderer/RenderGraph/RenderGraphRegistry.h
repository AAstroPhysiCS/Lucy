#pragma once

#include <variant>

#include "Renderer/Memory/Memory.h"
#include "Renderer/Device/RenderDeviceResource.h"
#include "Renderer/RenderPass.h"

#include "RenderGraphResource.h"

namespace Lucy {

    class Image;

    enum class RGResourceType : uint8_t {
        Internal,
        External,
        ExternalTransient
    };

    struct RGImageData {
        RenderPassLoadStoreAttachments LoadStoreAttachment{};
        bool IsDepth = false;
        bool InFlightMode = false;
    };

    struct RGBufferData {
        bool InFlightMode = false;
    };

    using RGResourceData = std::variant<RGImageData, RGBufferData>;

    struct RGResourceEntry {
        std::vector<RenderDeviceResourceHandle> ResourceHandles; //why vector? bcs we might have multiple buffers aka buffers per frame in flight
        RGResourceType Type = RGResourceType::Internal;
        RGResourceData Data;

        [[nodiscard]] bool IsImage() const { return std::holds_alternative<RGImageData>(Data); }
        [[nodiscard]] bool IsBuffer() const { return std::holds_alternative<RGBufferData>(Data); }

        [[nodiscard]] bool IsExternal() const { return Type == RGResourceType::External; }
        [[nodiscard]] bool IsExternalTransient() const { return Type == RGResourceType::ExternalTransient; }

        [[nodiscard]] RGImageData& GetImageData() { return std::get<RGImageData>(Data); }
        [[nodiscard]] const RGImageData& GetImageData() const { return std::get<RGImageData>(Data); }

        [[nodiscard]] RGBufferData& GetBufferData() { return std::get<RGBufferData>(Data); }
        [[nodiscard]] const RGBufferData& GetBufferData() const { return std::get<RGBufferData>(Data); }
    };

    using RGResources = std::unordered_map<RenderGraphResource, RGResourceEntry>;

    class RenderGraphRegistry final {
    public:
        RenderGraphRegistry() = default;
        ~RenderGraphRegistry() = default;

        RenderGraphRegistry(const RenderGraphRegistry& other) = delete;
        RenderGraphRegistry(RenderGraphRegistry&& other) noexcept = delete;
        RenderGraphRegistry& operator=(const RenderGraphRegistry& other) = delete;
        RenderGraphRegistry& operator=(RenderGraphRegistry&& other) noexcept = delete;

        void Flush();

        void ImportExternalResource(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle, RGResourceData data = {});
        void ImportExternalResource(const RenderGraphResource& rgResource, const std::vector<RenderDeviceResourceHandle>& handles, RGResourceData data = {});
        void ImportExternalTransientResource(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle);

        void DeclareImage(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle, const RGImageData& imageData);
        void DeclareImage(const RenderGraphResource& rgResource, const std::vector<RenderDeviceResourceHandle>& handles, const RGImageData& imageData);
        void DeclareBuffer(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle, const RGBufferData& bufferData);
        void DeclareBuffer(const RenderGraphResource& rgResource, const std::vector<RenderDeviceResourceHandle>& handles, const RGBufferData& bufferData);

        [[nodiscard]] bool Contains(const RenderGraphResource& rgResource) const;

        [[nodiscard]] Ref<Image> GetImage(const RenderGraphResource& rgResource);
        [[nodiscard]] Ref<Image> GetImage(const RenderGraphResource& rgResource) const;

        [[nodiscard]] Ref<RenderDeviceBuffer> GetBuffer(const RenderGraphResource& rgResource);
        [[nodiscard]] Ref<RenderDeviceBuffer> GetBuffer(const RenderGraphResource& rgResource) const;
    private:
        [[nodiscard]] RGResourceEntry& GetResourceEntry(const RenderGraphResource& rgResource) { return m_Resources.at(rgResource); }
        [[nodiscard]] const RGResourceEntry& GetResourceEntry(const RenderGraphResource& rgResource) const { return m_Resources.at(rgResource); }

        RGResources m_Resources;

        friend class RenderGraph;
    };
}