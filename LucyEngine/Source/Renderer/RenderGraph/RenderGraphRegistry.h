#pragma once

#include <variant>

#include "Renderer/Memory/Memory.h"
#include "Renderer/Device/RenderResource.h"
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
    };

    struct RGBufferData {
        // Alignment, size, usage flags, etc.
    };

    using RGResourceData = std::variant<RGImageData, RGBufferData>;

    struct RGResourceEntry {
        RenderResourceHandle ResourceHandle;
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

        void Flush();

        void ImportExternalResource(const RenderGraphResource& rgResource, RenderResourceHandle handle);
        void ImportExternalTransientResource(const RenderGraphResource& rgResource, RenderResourceHandle handle);

        void DeclareImage(const RenderGraphResource& rgResource, RenderResourceHandle handle, const RGImageData& imageData);
        void DeclareBuffer(const RenderGraphResource& rgResource, RenderResourceHandle handle, const RGBufferData& bufferData);

        [[nodiscard]] bool Contains(const RenderGraphResource& rgResource) const;

        [[nodiscard]] Ref<Image> GetImage(const RenderGraphResource& rgResource);
        [[nodiscard]] Ref<Image> GetImage(const RenderGraphResource& rgResource) const;

        [[nodiscard]] Ref<RenderResource> GetBuffer(const RenderGraphResource& rgResource);
        [[nodiscard]] Ref<RenderResource> GetBuffer(const RenderGraphResource& rgResource) const;
    private:
        [[nodiscard]] RGResourceEntry& GetResourceEntry(const RenderGraphResource& rgResource) { return m_Resources.at(rgResource); }
        [[nodiscard]] const RGResourceEntry& GetResourceEntry(const RenderGraphResource& rgResource) const { return m_Resources.at(rgResource); }

        RGResources m_Resources;

        friend class RenderGraph;
    };
}