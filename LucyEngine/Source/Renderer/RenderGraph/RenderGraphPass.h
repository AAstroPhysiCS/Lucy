#pragma once

#include "RenderGraphResource.h"

#include "Renderer/Device/RenderDevice.h"

namespace Lucy {

	class RenderCommandList;

	class RenderGraphRegistry;
	class RenderGraphBuilder;

	enum class RenderGraphExecutionPolicy : uint8_t {
		Once,
		Always
	};

	using RenderGraphExecuteFunc = std::function<void(RenderGraphRegistry&, RenderCommandList&)>;
	using RenderGraphSetupFunc = std::function<RenderGraphExecuteFunc(RenderGraphBuilder&)>;

	enum class RenderGraphPassState : uint8_t {
		New,
		Waiting,
		Runnable,
		Executed,
		Terminated
	};

	enum class RenderGraphResourceAccess : uint8_t {
		None,

		ColorAttachmentWrite,
		DepthAttachmentWrite,
		
		ShaderSampledRead,
		StorageRead,
		StorageWrite,
		
		TransferRead,
		TransferWrite,
		
		VertexRead, //TODO: maybe we dont need this?
		IndexRead, //TODO: maybe we dont need this?
		IndirectRead,

		Present,
	};

	enum class RenderGraphResourceType : uint8_t {
		Buffer,
		Image
	};

	struct RenderGraphResourceAddInfo {
		RenderGraphResource Resource = UndefinedRenderGraphResource;
		RenderGraphResourceType Type = RenderGraphResourceType::Image;
		RenderGraphResourceAccess Access = RenderGraphResourceAccess::None;
		TargetQueueFamily QueueFamily = TargetQueueFamily::Graphics;
		bool IsExternal = false;
		bool IsTransient = false;
	};

	/*
	* Quick note to myself:
	* Intra: between passes of the same queue family. We can only do pipeline barriers here.
	* Inter: between passes of different queue family. We have to do ownership transfer + pipeline barriers here.
	*/

	struct RenderGraphIntraQueueTransition {
		RenderGraphResource Resource = UndefinedRenderGraphResource;
		RenderGraphResourceType ResourceType = RenderGraphResourceType::Image;
		TargetQueueFamily QueueFamily = TargetQueueFamily::Graphics;

		RenderGraphResourceAccess SrcAccess = RenderGraphResourceAccess::None;
		RenderGraphResourceAccess DstAccess = RenderGraphResourceAccess::None;

		RenderGraphPass* SrcPass = nullptr;
		RenderGraphPass* DstPass = nullptr;
	};

	struct RenderGraphInterQueueTransition {
		RenderGraphResource Resource = UndefinedRenderGraphResource;
		RenderGraphResourceType ResourceType = RenderGraphResourceType::Image;

		TargetQueueFamily SrcQueue = TargetQueueFamily::Graphics;
		TargetQueueFamily DstQueue = TargetQueueFamily::Graphics;

		RenderGraphResourceAccess SrcAccess = RenderGraphResourceAccess::None;
		RenderGraphResourceAccess DstAccess = RenderGraphResourceAccess::None;

		RenderGraphPass* SrcPass = nullptr;
		RenderGraphPass* DstPass = nullptr;
	};

	struct RenderGraphBatch {
		std::vector<RenderGraphPass*> Passes;
		std::vector<RenderGraphInterQueueTransition> IncomingInterQueueTransitions; //acquire
		std::vector<RenderGraphInterQueueTransition> OutgoingInterQueueTransitions; //release

		std::vector<RenderGraphIntraQueueTransition> IntraQueueTransition;
	};

	using RenderGraphBatches = std::vector<RenderGraphBatch>;

	struct RenderGraphPassCreateInfo {
		RenderGraphSetupFunc SetupFunc;
		RenderGraphRegistry& Registry;
		TargetQueueFamily TargetQueueFamily;
		std::string Name = "Unnamed RenderGraphPass";
	};

	using RGRenderTargetElements = std::vector<RenderGraphResource>;
	using RGUsedResourceElements = std::vector<RenderGraphResourceAddInfo>;

	class RenderGraphPass final {
	public:
		RenderGraphPass(const RenderGraphPassCreateInfo& createInfo);
		~RenderGraphPass() = default;

		void Execute(RenderCommandList& cmdList);
		void Setup(RenderGraphBuilder& build);
		
		void AddRenderTarget(const RenderGraphResource& renderTargetToAdd);
		void AddResourceRead(const RenderGraphResourceAddInfo& addInfo);
		void AddResourceWrite(const RenderGraphResourceAddInfo& addInfo);

		void SetViewportArea(uint32_t width, uint32_t height);
		void OnViewportResize(uint32_t width, uint32_t height);

		void SetInFlightMode(bool mode);
		void SetState(RenderGraphPassState state);
		void SetClearColor(ClearColor clearColor);
		void SetExecutionPolicy(RenderGraphExecutionPolicy policy);

		inline bool operator==(const RenderGraphPass& other) const { return m_CreateInfo.Name.compare(other.m_CreateInfo.Name) == 0; }

		inline const RGRenderTargetElements& GetRenderTargets() const { return m_RenderTargets; }

		inline const RGUsedResourceElements& GetResourceReads() const { return m_ResourceReads; }
		inline const RGUsedResourceElements& GetResourceWrites() const { return m_ResourceWrites; }

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

		inline RenderGraphExecutionPolicy GetExecutionPolicy() const { return m_ExecutionPolicy; }
		inline RenderGraphPassState GetCurrentState() const { return m_State; }
		inline const std::string& GetName() const { return m_CreateInfo.Name; }
	private:
		RenderGraphExecuteFunc m_ExecuteFunc;
		RenderGraphPassCreateInfo m_CreateInfo;

		RGRenderTargetElements m_RenderTargets;

		RGUsedResourceElements m_ResourceReads;
		RGUsedResourceElements m_ResourceWrites;

		RenderGraphPassState m_State = RenderGraphPassState::New;
		RenderGraphExecutionPolicy m_ExecutionPolicy = RenderGraphExecutionPolicy::Always;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		bool m_PassIsInFlightMode = false;

		ClearColor m_ClearColor;
	};
}