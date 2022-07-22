#pragma once

namespace Lucy {

	struct CommandPoolCreateInfo {
		uint32_t PoolFlags = 0;
		uint32_t Level = 0;
		uint32_t CommandBufferCount = 0;
	};

	class CommandPool {
	public:
		CommandPool(const CommandPoolCreateInfo& createInfo);
		virtual ~CommandPool() = default;

		static Ref<CommandPool> Create(const CommandPoolCreateInfo& createInfo);
		
		virtual void Destroy() = 0;
		virtual void Recreate() = 0;
	protected:
		virtual void Allocate() = 0;

		CommandPoolCreateInfo m_CreateInfo;
	};
}

