#pragma once

#include "Renderer/Memory/Memory.h"
#include "RenderResource.h"
#include "Utilities/Random.h"

namespace Lucy {

	template <typename TResource>
	concept IsRenderResource = std::is_base_of_v<MemoryTrackable, TResource> && std::is_base_of_v<RenderResource, TResource>;

	class RenderDeviceResourceManager final {
		RenderDeviceResourceManager() = default;
		~RenderDeviceResourceManager() = default;

		template <typename TResource>
		inline RenderResourceHandle PushResource(const TResource& r) {
			static UniformRandom<uint64_t> randomGen;

			RenderResourceHandle handle = randomGen.NextValue();
			m_Resources.emplace(handle, std::move(r));
			return handle;
		}

		inline bool ResourceExists(RenderResourceHandle handle) const {
			return m_Resources.contains(handle);
		}

		inline Ref<RenderResource> GetResource(RenderResourceHandle handle) {
			LUCY_ASSERT(ResourceExists(handle), "Resource handle {0} could not be found!", handle);
			return m_Resources.at(handle);
		}

		//frees the resource and deletes/invalidates the handle
		inline void RTDestroyResource(RenderResourceHandle& handle) {
			m_Resources.at(handle)->RTDestroyResource();
			m_Resources.erase(handle);
			handle = InvalidRenderResourceHandle;
		}

		inline void Clear() {
			m_Resources.clear();
		}

		friend class RenderDevice;

		std::unordered_map<RenderResourceHandle, Ref<RenderResource>> m_Resources;
	};
}