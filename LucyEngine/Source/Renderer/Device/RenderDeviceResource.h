#pragma once

#include "Utilities/GenerationalPool.h"

#include "RenderDeviceHandles.h"

namespace Lucy {

	class RenderDevice;

	class RenderDeviceResource : public MemoryTrackable {
	public:
		virtual ~RenderDeviceResource() = default;

		RenderDeviceResource(const RenderDeviceResource&) = delete;
		RenderDeviceResource& operator=(const RenderDeviceResource&) = delete;
		RenderDeviceResource(RenderDeviceResource&&) = delete;
		RenderDeviceResource& operator=(RenderDeviceResource&&) = delete;

		inline bool IsInitialized() const { return m_Handle; }
		inline const std::string& GetDebugName() const { return m_DebugName; }
	protected:
		RenderDeviceResource(std::string_view name)
			: m_DebugName(name) {}

		RenderDeviceResourceHandle& GetMyHandle() { return m_Handle; }
	private:
		inline void SetInitialized(RenderDeviceResourceHandle handle) { m_Handle = handle; }

		virtual void RTDestroyResource(RenderDevice* device) = 0;

		std::string m_DebugName;
		RenderDeviceResourceHandle m_Handle{};

		friend class RenderDeviceResourceManager;
	};
}