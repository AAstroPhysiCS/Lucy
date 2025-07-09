#pragma once

namespace Lucy {

	using RenderResourceHandle = uint64_t;

	static inline const auto InvalidRenderResourceHandle = RenderResourceHandle(~0uLL);

	class RenderResource : public MemoryTrackable {
	public:
		RenderResource(const char* name) 
			: m_DebugName(name) {
		}
		virtual ~RenderResource() = default;

		inline const std::string& GetDebugName() const { return m_DebugName; }
	private:
		virtual void RTDestroyResource() = 0;

		std::string m_DebugName;

		friend class RenderDeviceResourceManager;
	};
}