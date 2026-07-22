#pragma once

#include "Utilities/GenerationalPool.h"

namespace Lucy {

	class RenderDeviceResource : public MemoryTrackable {
	public:
		virtual ~RenderDeviceResource() = default;

		RenderDeviceResource(const RenderDeviceResource&) = delete;
		RenderDeviceResource& operator=(const RenderDeviceResource&) = delete;
		RenderDeviceResource(RenderDeviceResource&&) = delete;
		RenderDeviceResource& operator=(RenderDeviceResource&&) = delete;

		inline bool IsInitialized() const { return m_IsInitialized; }
		inline const std::string& GetDebugName() const { return m_DebugName; }
	protected:
		RenderDeviceResource(std::string_view name)
			: m_DebugName(name) {}
	private:
		inline void SetInitialized(bool initialized) { m_IsInitialized = initialized; }

		virtual void RTDestroyResource() = 0;

		std::string m_DebugName;
		bool m_IsInitialized = false;

		friend class RenderDeviceResourceManager;
	};
}