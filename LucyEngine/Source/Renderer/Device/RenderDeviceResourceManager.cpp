#include "lypch.h"
#include "RenderDeviceResourceManager.h"

namespace Lucy {

	RenderDeviceResourceManager::RenderDeviceResourceManager(RenderDevice* device) 
		: m_RenderDevice(device) {
	}

	void RenderDeviceResourceManager::RTDestroyResource(RenderDeviceResourceHandle& handle) {
		LUCY_ASSERT(m_Resources.IsValid(handle), "Invalid render device resource handle.");

		Ref<RenderDeviceResource>& resource = m_Resources.Get(handle);
		resource->RTDestroyResource(m_RenderDevice);

		//m_Resources.Destroy(handle);
		//handle = {};
	}
}
