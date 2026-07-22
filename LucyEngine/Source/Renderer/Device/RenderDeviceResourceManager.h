#pragma once

#include "Renderer/Memory/Memory.h"
#include "RenderDeviceResource.h"
#include "RenderDeviceHandles.h"

#include "Utilities/UUID.h"
#include "Utilities/GenerationalPool.h"

namespace Lucy {

	template <typename TResource>
	concept IsRenderResource = std::is_base_of_v<MemoryTrackable, TResource> && std::is_base_of_v<RenderDeviceResource, TResource>;

	using RenderDeviceResourcePool = GenerationalPool<RenderDeviceResourceHandle, Ref<RenderDeviceResource>>;

	class RenderDeviceResourceManager final {
		RenderDeviceResourceManager() = default;
		~RenderDeviceResourceManager() = default;

		RenderDeviceResourceManager(const RenderDeviceResourceManager&) = delete;
		RenderDeviceResourceManager& operator=(const RenderDeviceResourceManager&) = delete;
		RenderDeviceResourceManager(RenderDeviceResourceManager&&) = delete;
		RenderDeviceResourceManager& operator=(RenderDeviceResourceManager&&) = delete;

		template<IsRenderResource TResource>
		[[nodiscard]] RenderDeviceResourceHandle PushResource(Ref<TResource> resource) {
			LUCY_ASSERT(resource, "Cannot push nullptr render device resource.");
			resource->SetInitialized(true);
			return m_Resources.Create(std::move(resource));
		}

		[[nodiscard]] bool ResourceExists(RenderDeviceResourceHandle handle) const {
			return m_Resources.IsValid(handle);
		}

		[[nodiscard]] Ref<RenderDeviceResource> GetResource(RenderDeviceResourceHandle handle) {
			return m_Resources.Get(handle);
		}
		
		[[nodiscard]] const Ref<RenderDeviceResource>& GetResource(RenderDeviceResourceHandle handle) const {
			return m_Resources.Get(handle);
		}
		
		void RTDestroyResource(RenderDeviceResourceHandle& handle) {
			LUCY_ASSERT(m_Resources.IsValid(handle), "Invalid render device resource handle.");

			Ref<RenderDeviceResource>& resource = m_Resources.Get(handle);
			resource->RTDestroyResource();

			m_Resources.Destroy(handle);
			handle = {};
		}
	private:
		RenderDeviceResourcePool m_Resources;

		friend class RenderDevice;
	};
}