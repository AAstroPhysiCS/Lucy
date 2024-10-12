#pragma once

namespace Lucy {

	struct CommandPoolCreateInfo {
		uint32_t CommandBufferCount = 0;
		uint32_t Level = 0;
		uint32_t PoolFlags = 0;

		Ref<RenderDevice> RenderDevice = nullptr;
	};

	class CommandPool : public MemoryTrackable {
	public:
		CommandPool(const CommandPoolCreateInfo& createInfo) 
			: m_CreateInfo(createInfo) {
		}
		virtual ~CommandPool() = default;

		virtual void* GetCurrentFrameCommandBuffer() = 0;

		virtual void Recreate() = 0;
		virtual void Destroy() = 0;
	protected:
		CommandPoolCreateInfo m_CreateInfo;
	};
}