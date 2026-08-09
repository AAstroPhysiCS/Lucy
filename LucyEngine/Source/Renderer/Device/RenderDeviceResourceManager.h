#pragma once

#include "Renderer/Memory/Memory.h"
#include "RenderDeviceResource.h"
#include "RenderDeviceHandles.h"

#include "Utilities/UUID.h"
#include "Utilities/GenerationalPool.h"

namespace Lucy {

	class RenderDevice;

	template <typename TResource>
	concept IsRenderResource = std::is_base_of_v<MemoryTrackable, TResource> && std::is_base_of_v<RenderDeviceResource, TResource>;

	using RenderDeviceResourcePool = GenerationalPool<RenderDeviceResourceHandle, Ref<RenderDeviceResource>>;

	class RenderDeviceResourceManager final {
		RenderDeviceResourceManager(RenderDevice* device);
		~RenderDeviceResourceManager() = default;

		RenderDeviceResourceManager(const RenderDeviceResourceManager&) = delete;
		RenderDeviceResourceManager& operator=(const RenderDeviceResourceManager&) = delete;
		RenderDeviceResourceManager(RenderDeviceResourceManager&&) = delete;
		RenderDeviceResourceManager& operator=(RenderDeviceResourceManager&&) = delete;

		template<IsRenderResource TResource>
		[[nodiscard]] RenderDeviceResourceHandle PushResource(Ref<TResource> resource) {
			LUCY_ASSERT(resource, "Cannot push nullptr render device resource.");
			auto handle = m_Resources.Create(std::move(resource));
			m_Resources.Back().Data->SetInitialized(handle);
			return handle;
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
		
		void RTDestroyResource(RenderDeviceResourceHandle& handle);
	private:
		RenderDeviceResourcePool m_Resources;
		RenderDevice* m_RenderDevice = nullptr;

		friend class RenderDevice;
	};
}