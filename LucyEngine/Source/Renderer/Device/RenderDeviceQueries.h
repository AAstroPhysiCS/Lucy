#pragma once

#include "RenderDevice.h"

namespace Lucy {

	class CommandPool;

	enum class RenderDeviceQueryType : uint8_t {
		Timestamp,
		Pipeline,
		//TODO: other query type support...
	};

	struct RenderDeviceQueryCreateInfo {
		Ref<RenderDevice> Device = nullptr;
		uint32_t QueryCount = 0;
		RenderDeviceQueryType QueryType;
	};

	class RenderDeviceQuery {
	public:
		static Unique<RenderDeviceQuery> Create(const RenderDeviceQueryCreateInfo& createInfo);

		RenderDeviceQuery(const RenderDeviceQueryCreateInfo& createInfo);
		virtual ~RenderDeviceQuery() = default;

		virtual uint32_t Begin(Ref<CommandPool> cmdPool) = 0;
		virtual uint32_t End(Ref<CommandPool> cmdPool) = 0;
		virtual void ResetPool(Ref<CommandPool> cmdPool) = 0;
		virtual std::vector<uint64_t> GetQueryResults() = 0;
		virtual void Destroy() = 0;

		inline const RenderDeviceQueryCreateInfo& GetCreateInfo() const { return m_CreateInfo; }
	private:
		RenderDeviceQueryCreateInfo m_CreateInfo;
	};

	class VulkanRenderDeviceQuery final : public RenderDeviceQuery {
	public:
		VulkanRenderDeviceQuery(const RenderDeviceQueryCreateInfo& createInfo);
		virtual ~VulkanRenderDeviceQuery() = default;

		uint32_t Begin(Ref<CommandPool> cmdPool) final override;
		uint32_t End(Ref<CommandPool> cmdPool) final override;
		void ResetPool(Ref<CommandPool> cmdPool) final override;
		std::vector<uint64_t> GetQueryResults() final override;
		void Destroy() final override;
	private:
		std::vector<VkQueryPool> m_QueryPools;
		uint32_t m_ActiveQueryIndex = 0uLL;
	};
}