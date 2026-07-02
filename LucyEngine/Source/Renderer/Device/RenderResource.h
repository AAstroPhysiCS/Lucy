#pragma once

namespace Lucy {

	using RenderResourceHandle = uint64_t;

	static inline const auto InvalidRenderResourceHandle = RenderResourceHandle(~0uLL);

	class RenderResource : public MemoryTrackable {
	public:
		RenderResource(std::string_view name) 
			: m_DebugName(name) {
		}
		virtual ~RenderResource() = default;

		inline bool IsInitialized() const { return m_IsInitialized; }
		inline const std::string& GetDebugName() const { return m_DebugName; }
	private:
		inline void SetInitialized(bool initialized) { m_IsInitialized = initialized; }

		virtual void RTDestroyResource() = 0;

		std::string m_DebugName;
		bool m_IsInitialized = false;

		friend class RenderDeviceResourceManager;
	};
}