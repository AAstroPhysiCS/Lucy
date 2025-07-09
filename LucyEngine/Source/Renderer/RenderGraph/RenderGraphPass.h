#pragma once

namespace Lucy {

	class RenderCommandList;

	class RenderGraphResource;
	class RenderGraphRegistry;
	class RenderGraphBuilder;

	using RenderGraphExecuteFunc = std::function<void(RenderGraphRegistry&, RenderCommandList&)>;
	using RenderGraphSetupFunc = std::function<RenderGraphExecuteFunc(RenderGraphBuilder&)>;

	enum class RenderGraphPassState : uint8_t {
		New,
		Waiting,
		Runnable,
		Terminated
	};

	struct RenderGraphPassCreateInfo {
		RenderGraphSetupFunc SetupFunc;
		RenderGraphRegistry& Registry;
		TargetQueueFamily TargetQueueFamily;
		std::string Name = "Unnamed RenderGraphPass";
	};

	using RGRenderTargetElements = std::vector<RenderGraphResource>;

	class RenderGraphPass final {
	public:
		RenderGraphPass(const RenderGraphPassCreateInfo& createInfo);
		~RenderGraphPass() = default;

		void Execute(RenderCommandList& cmdList);
		void Setup(RenderGraphBuilder& build);
		void AddRenderTarget(const RenderGraphResource& renderTargetToAdd);

		void SetViewportArea(uint32_t width, uint32_t height);
		void OnViewportResize(uint32_t width, uint32_t height);

		void SetInFlightMode(bool mode);
		void SetState(RenderGraphPassState state);
		void SetClearColor(ClearColor clearColor);

		inline bool operator==(const RenderGraphPass& other) const { return m_CreateInfo.Name.compare(other.m_CreateInfo.Name) == 0; }

		inline const RGRenderTargetElements& GetRenderTargets() const { return m_RenderTargets; }

		inline ClearColor GetClearColor() { return m_ClearColor; }

		inline auto GetViewportArea() const {
			struct Area {
				uint32_t width;
				uint32_t height;

				inline bool operator>(int32_t size) const { return width > size && height > size; }
			};
			return Area{ m_ViewportWidth, m_ViewportHeight };
		}
		inline bool IsInFlightMode() const { return m_PassIsInFlightMode; }

		inline TargetQueueFamily GetTargetQueueFamily() const { return m_CreateInfo.TargetQueueFamily; }

		inline RenderGraphPassState GetCurrentState() const { return m_State; }
		inline const std::string& GetName() const { return m_CreateInfo.Name; }
	private:
		RenderGraphExecuteFunc m_ExecuteFunc;
		RenderGraphPassCreateInfo m_CreateInfo;

		RGRenderTargetElements m_RenderTargets;

		RenderGraphPassState m_State = RenderGraphPassState::New;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		bool m_PassIsInFlightMode = false;

		ClearColor m_ClearColor;
	};
}