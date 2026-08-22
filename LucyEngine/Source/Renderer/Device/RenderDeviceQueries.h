#pragma once

namespace Lucy {

	class RenderDevice;
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

	class RenderDeviceQuery : public MemoryTrackable {
	public:
		static Ref<RenderDeviceQuery> Create(const RenderDeviceQueryCreateInfo& createInfo);

		RenderDeviceQuery(const RenderDeviceQueryCreateInfo& createInfo);
		virtual ~RenderDeviceQuery() = default;

		RenderDeviceQuery(const RenderDeviceQuery&) = delete;
		RenderDeviceQuery& operator=(const RenderDeviceQuery&) = delete;
		RenderDeviceQuery(RenderDeviceQuery&&) = delete;
		RenderDeviceQuery& operator=(RenderDeviceQuery&&) = delete;

		virtual uint32_t RTBegin(Ref<CommandPool> cmdPool) = 0;
		virtual uint32_t RTEnd(Ref<CommandPool> cmdPool) = 0;
		virtual void ResetPoolByIndex(uint32_t frameIndex) = 0;
		virtual void RTResetPoolByIndex(Ref<CommandPool> commandPool, uint32_t frameIndex) = 0;
		virtual std::vector<uint64_t> GetQueryResults(uint32_t frameIndex) = 0;
		virtual void Destroy() = 0;

		inline const RenderDeviceQueryCreateInfo& GetCreateInfo() const { return m_CreateInfo; }
	private:
		RenderDeviceQueryCreateInfo m_CreateInfo;
	};

	class VulkanRenderDeviceQuery final : public RenderDeviceQuery {
	public:
		VulkanRenderDeviceQuery(const RenderDeviceQueryCreateInfo& createInfo);
		virtual ~VulkanRenderDeviceQuery() = default;

		VulkanRenderDeviceQuery(const VulkanRenderDeviceQuery&) = delete;
		VulkanRenderDeviceQuery& operator=(const VulkanRenderDeviceQuery&) = delete;
		VulkanRenderDeviceQuery(VulkanRenderDeviceQuery&&) = delete;
		VulkanRenderDeviceQuery& operator=(VulkanRenderDeviceQuery&&) = delete;

		uint32_t RTBegin(Ref<CommandPool> cmdPool) final override;
		uint32_t RTEnd(Ref<CommandPool> cmdPool) final override;
		
		void ResetPoolByIndex(uint32_t frameIndex) final override;
		void RTResetPoolByIndex(Ref<CommandPool> commandPool, uint32_t frameIndex) final override;

		std::vector<uint64_t> GetQueryResults(uint32_t frameIndex) final override;
		void Destroy() final override;
	private:
		std::vector<VkQueryPool> m_QueryPools;
		std::vector<uint32_t> m_ActiveQueryIndex;
	};
}