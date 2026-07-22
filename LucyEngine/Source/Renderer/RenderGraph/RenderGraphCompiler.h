#pragma once

#include "RenderGraphResource.h"
#include "RenderGraphPass.h"

#include "Renderer/ExecutionBatch.h"
#include "Renderer/Device/RenderDevice.h"

#include "Utilities/UUID.h"

namespace Lucy {

	class RenderGraph;

	class RenderGraphCompiler {
	public:
		RenderGraphCompiler(const RenderGraph& renderGraph, const Ref<RenderDevice>& device);
		virtual ~RenderGraphCompiler() = default;

		RenderGraphCompiler(const RenderGraphCompiler& other) = delete;
		RenderGraphCompiler(RenderGraphCompiler&& other) noexcept = delete;
		RenderGraphCompiler& operator=(const RenderGraphCompiler& other) = delete;
		RenderGraphCompiler& operator=(RenderGraphCompiler&& other) noexcept = delete;

		virtual std::vector<ExecutionBatch> Compile(const RenderGraphBatches& batches) = 0;
	protected:
		inline Ref<RenderDevice> GetRenderDevice() const { return m_RenderDevice; }
		inline const RenderGraph& GetRenderGraph() const { return m_RenderGraph; }

		inline IDProvider<ExecutionBatchID, false>& GetExecutionBatchIDProvider() { return m_ExecutionBatchIDProvider; }
	private:
		const RenderGraph& m_RenderGraph;

		Ref<RenderDevice> m_RenderDevice = nullptr;
		IDProvider<ExecutionBatchID, false> m_ExecutionBatchIDProvider;
	};

#pragma region VulkanRenderGraphCompiler
	inline constexpr static VkPipelineStageFlags2 ToStageMask(RenderGraphResourceAccess access, TargetQueueFamily queueFamily) {
		switch (access) {
			case RenderGraphResourceAccess::ColorAttachmentWrite:
				return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			case RenderGraphResourceAccess::DepthAttachmentWrite:
				return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
					VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			case RenderGraphResourceAccess::ShaderSampledRead:
				switch (queueFamily) {
					case TargetQueueFamily::Graphics:
						return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
							VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
					case TargetQueueFamily::Compute:
						return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
					default:
						LUCY_ASSERT(false, "Sampled read not valid on this queue");
						return VK_PIPELINE_STAGE_2_NONE;
				}
			case RenderGraphResourceAccess::StorageRead:
			case RenderGraphResourceAccess::StorageWrite:
				switch (queueFamily) {
					case TargetQueueFamily::Graphics:
						return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
					case TargetQueueFamily::Compute:
						return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
					default:
						LUCY_ASSERT(false, "Storage access not valid on this queue");
						return VK_PIPELINE_STAGE_2_NONE;
				}
			case RenderGraphResourceAccess::TransferRead:
			case RenderGraphResourceAccess::TransferWrite:
				return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			case RenderGraphResourceAccess::VertexRead:
				return VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
			case RenderGraphResourceAccess::IndexRead:
				return VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
			case RenderGraphResourceAccess::IndirectRead:
				return VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
			case RenderGraphResourceAccess::Present:
				return VK_PIPELINE_STAGE_2_NONE;
			default:
				return VK_PIPELINE_STAGE_2_NONE;
		}
	}

	inline constexpr static VkAccessFlags2 ToAccessMask(RenderGraphResourceAccess access) {
		switch (access) {
			case RenderGraphResourceAccess::ColorAttachmentWrite: return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			case RenderGraphResourceAccess::DepthAttachmentWrite: return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			case RenderGraphResourceAccess::ShaderSampledRead: return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			case RenderGraphResourceAccess::StorageRead: return VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
			case RenderGraphResourceAccess::StorageWrite: return VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			case RenderGraphResourceAccess::TransferRead: return VK_ACCESS_2_TRANSFER_READ_BIT;
			case RenderGraphResourceAccess::TransferWrite: return VK_ACCESS_2_TRANSFER_WRITE_BIT;
			case RenderGraphResourceAccess::VertexRead: return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
			case RenderGraphResourceAccess::IndexRead: return VK_ACCESS_2_INDEX_READ_BIT;
			case RenderGraphResourceAccess::IndirectRead: return VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
			default: return VK_ACCESS_2_NONE;
		}
	}

	inline constexpr static VkImageLayout ToImageLayout(RenderGraphResourceAccess access, bool isDepth) {
		switch (access) {
			case RenderGraphResourceAccess::ColorAttachmentWrite: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case RenderGraphResourceAccess::DepthAttachmentWrite: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			case RenderGraphResourceAccess::ShaderSampledRead: return isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case RenderGraphResourceAccess::StorageRead:
			case RenderGraphResourceAccess::StorageWrite: return VK_IMAGE_LAYOUT_GENERAL;
			case RenderGraphResourceAccess::TransferRead: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			case RenderGraphResourceAccess::TransferWrite: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			case RenderGraphResourceAccess::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			case RenderGraphResourceAccess::None: return VK_IMAGE_LAYOUT_UNDEFINED;
			default: return VK_IMAGE_LAYOUT_GENERAL;
		}
	}

	class VulkanRenderGraphCompiler : public RenderGraphCompiler {
	public:
		VulkanRenderGraphCompiler(const RenderGraph& renderGraph, const Ref<RenderDevice>& device);
		virtual ~VulkanRenderGraphCompiler() = default;

		std::vector<ExecutionBatch> Compile(const RenderGraphBatches& batches) final override;
	};
#pragma endregion VulkanRenderGraphCompiler
}
