#pragma once

namespace Lucy {

	enum class TargetQueueFamily : uint8_t;

	enum class CommandBufferSlotState : uint8_t {
		Ready,      // can be reset and begun
		Recording,  // between Begin and End
		Recorded,   // after End, before submit
		Pending     // submitted to GPU, waiting for frame completion
	};

	struct CommandPoolCreateInfo {
		uint32_t CommandBufferCount = 0;
		uint32_t Level = 0;
		uint32_t PoolFlags = 0;

		Ref<RenderDevice> RenderDevice = nullptr;
		TargetQueueFamily TargetQueueFamily;
	};

	class CommandPool : public MemoryTrackable {
	public:
		CommandPool(const CommandPoolCreateInfo& createInfo) 
			: m_CreateInfo(createInfo) {
			m_CommandStates.resize(m_CreateInfo.CommandBufferCount, CommandBufferSlotState::Ready);
		}
		virtual ~CommandPool() = default;

		inline CommandBufferSlotState GetState(uint32_t frameIndex) { return m_CommandStates[frameIndex]; }
		inline void SetState(uint32_t frameIndex, CommandBufferSlotState state) { m_CommandStates[frameIndex] = state; }

		inline uint32_t GetCommandBufferCount() const { return m_CreateInfo.CommandBufferCount; }

		virtual void* GetCommandBuffer(uint32_t frameIndex) = 0;

		virtual void Reset() = 0;
		virtual void ResetCommandBuffer(uint32_t frameIndex) = 0;
		virtual void Recreate() = 0;
		virtual void Destroy() = 0;
	protected:
		inline void SetAllState(CommandBufferSlotState state) {
			for (size_t i = 0; i < GetCommandBufferCount(); i++)
				m_CommandStates[i] = state;
		}

		CommandPoolCreateInfo m_CreateInfo;
	private:
		std::vector<CommandBufferSlotState> m_CommandStates;
	};
}