#include "lypch.h"
#include "RenderGraphCompiler.h"

#include "RenderGraph.h"

#include "Renderer/ExecutionBatch.h"

#include "Renderer/Device/VulkanRenderDevice.h"
#include "Renderer/Image/VulkanImage.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanSharedStorageBuffer.h"

namespace Lucy {

	RenderGraphCompiler::RenderGraphCompiler(const RenderGraph& renderGraph, const Ref<RenderDevice>& device) 
		: m_RenderGraph(renderGraph), m_RenderDevice(device) {
	}

	VulkanRenderGraphCompiler::VulkanRenderGraphCompiler(const RenderGraph& renderGraph, const Ref<RenderDevice>& device)
		: RenderGraphCompiler(renderGraph, device) {
	}

	std::vector<ExecutionBatch> VulkanRenderGraphCompiler::Compile(const RenderGraphBatches& batches) {
		GetExecutionBatchIDProvider().Reset();

		const auto& renderGraph = GetRenderGraph();
		auto renderDevice = GetRenderDevice();

		const auto GetQueueFamilyIndex = [vulkanDevice = renderDevice->As<VulkanRenderDevice>()](TargetQueueFamily queueFamily) {
			return vulkanDevice->GetQueue(queueFamily).Family;
		};

		const auto CreateImageRange = [](const Ref<VulkanImage>& image) {
			const bool isDepth = image->GetFormat() == ImageFormat::D32_SFLOAT;
			return VkImageSubresourceRange{
				.aspectMask = (VkImageAspectFlags)(isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = image->GetLayerCount()
			};
		};

		const auto CreateSameQueueImageBarrier = [&](TargetQueueFamily queueFamily, const Ref<VulkanImage>& image, RenderGraphResourceAccess srcAccess, RenderGraphResourceAccess dstAccess) {
			const bool isDepth = image->GetFormat() == ImageFormat::D32_SFLOAT;
			return VkImageMemoryBarrier2{
				VulkanAPI::VulkanPipelineBarrier(
					image->GetVulkanHandle(),
					ToImageLayout(srcAccess, isDepth),
					ToImageLayout(dstAccess, isDepth),
					CreateImageRange(image),
					ToStageMask(srcAccess, queueFamily),
					ToStageMask(dstAccess, queueFamily),
					ToAccessMask(srcAccess),
					ToAccessMask(dstAccess),
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED
				)
			};
		};

		const auto CreateReleaseImageBarrier = [&](TargetQueueFamily srcQueue, TargetQueueFamily dstQueue, const Ref<VulkanImage>& image, RenderGraphResourceAccess srcAccess, RenderGraphResourceAccess dstAccess) {
			const bool isDepth = image->GetFormat() == ImageFormat::D32_SFLOAT;
			return VkImageMemoryBarrier2{
				VulkanAPI::VulkanPipelineBarrier(
					image->GetVulkanHandle(),
					ToImageLayout(srcAccess, isDepth),
					ToImageLayout(dstAccess, isDepth),
					CreateImageRange(image),
					ToStageMask(srcAccess, srcQueue),
					VK_PIPELINE_STAGE_2_NONE,
					ToAccessMask(srcAccess),
					VK_ACCESS_2_NONE,
					GetQueueFamilyIndex(srcQueue),
					GetQueueFamilyIndex(dstQueue)
				)
			};
		};

		const auto CreateAcquireImageBarrier = [&](TargetQueueFamily srcQueue, TargetQueueFamily dstQueue, const Ref<VulkanImage>& image, RenderGraphResourceAccess srcAccess, RenderGraphResourceAccess dstAccess) {
			const bool isDepth = image->GetFormat() == ImageFormat::D32_SFLOAT;
			return VkImageMemoryBarrier2{
				VulkanAPI::VulkanPipelineBarrier(
					image->GetVulkanHandle(),
					ToImageLayout(srcAccess, isDepth),
					ToImageLayout(dstAccess, isDepth),
					CreateImageRange(image),
					VK_PIPELINE_STAGE_2_NONE,
					ToStageMask(dstAccess, dstQueue),
					VK_ACCESS_2_NONE,
					ToAccessMask(dstAccess),
					GetQueueFamilyIndex(srcQueue),
					GetQueueFamilyIndex(dstQueue)
				)
			};
		};

		const auto CreateSameQueueBufferBarrier = [](TargetQueueFamily queueFamily, const Ref<VulkanSharedStorageBuffer>& buffer, RenderGraphResourceAccess srcAccess, RenderGraphResourceAccess dstAccess) {
			return VkBufferMemoryBarrier2{
				VulkanAPI::VulkanPipelineBarrier(
					buffer->GetVulkanBufferHandle(Renderer::GetCurrentFrameIndex()),
					ToStageMask(srcAccess, queueFamily),
					ToStageMask(dstAccess, queueFamily),
					ToAccessMask(srcAccess),
					ToAccessMask(dstAccess),
					0,
					VK_WHOLE_SIZE,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED
				)
			};
		};

		const auto CreateReleaseBufferBarrier = [&](TargetQueueFamily srcQueue, TargetQueueFamily dstQueue, const Ref<VulkanSharedStorageBuffer>& buffer, RenderGraphResourceAccess srcAccess) {
			return VkBufferMemoryBarrier2{
				VulkanAPI::VulkanPipelineBarrier(
					buffer->GetVulkanBufferHandle(Renderer::GetCurrentFrameIndex()),
					ToStageMask(srcAccess, srcQueue),
					VK_PIPELINE_STAGE_2_NONE,
					ToAccessMask(srcAccess),
					VK_ACCESS_2_NONE,
					0,
					VK_WHOLE_SIZE,
					GetQueueFamilyIndex(srcQueue),
					GetQueueFamilyIndex(dstQueue)
				)
			};
		};

		const auto CreateAcquireBufferBarrier = [&](TargetQueueFamily srcQueue, TargetQueueFamily dstQueue, const Ref<VulkanSharedStorageBuffer>& buffer, RenderGraphResourceAccess dstAccess) {
			return VkBufferMemoryBarrier2{
				VulkanAPI::VulkanPipelineBarrier(
					buffer->GetVulkanBufferHandle(Renderer::GetCurrentFrameIndex()),
					VK_PIPELINE_STAGE_2_NONE,
					ToStageMask(dstAccess, dstQueue),
					VK_ACCESS_2_NONE,
					ToAccessMask(dstAccess),
					0,
					VK_WHOLE_SIZE,
					GetQueueFamilyIndex(srcQueue),
					GetQueueFamilyIndex(dstQueue)
				)
			};
		};

		std::vector<ExecutionBatch> result;
		result.reserve(batches.size());

		auto& idProvider = GetExecutionBatchIDProvider();

		const auto LogImageBarrier =
			[](std::string_view batchName,
			const VkImageMemoryBarrier2& barrier) {
			LUCY_INFO(
				"Batch '{}': image={}, oldLayout={}, newLayout={}, "
				"srcStage={}, dstStage={}, srcAccess={}, dstAccess={}, "
				"srcFamily={}, dstFamily={}",
				batchName,
				static_cast<const void*>(barrier.image),
				static_cast<int>(barrier.oldLayout),
				static_cast<int>(barrier.newLayout),
				barrier.srcStageMask,
				barrier.dstStageMask,
				barrier.srcAccessMask,
				barrier.dstAccessMask,
				barrier.srcQueueFamilyIndex,
				barrier.dstQueueFamilyIndex
			);
		};

		for (const auto& rgBatch : batches) {
			TargetQueueFamily family = rgBatch.Passes[0]->GetTargetQueueFamily();

			ExecutionBatch batch = VulkanExecutionBatch{
				.QueueFamily = family,
				.Passes = rgBatch.Passes,
			};
			batch.ID = idProvider.RequestID();

			auto& vkBatch = batch.AsVulkanBatch();

			// Same-queue barriers go into pre-batch
			for (const auto& br : rgBatch.IntraQueueTransition) {
				if (br.ResourceType == RenderGraphResourceType::Image) {
					auto image = renderGraph.GetImageByRGResource(br.Resource)->As<VulkanImage>();
					vkBatch.PreBatchBarrier.ImageBarriers.push_back(
						VulkanImageMemoryBarrier{ image, CreateSameQueueImageBarrier(family, image, br.SrcAccess, br.DstAccess) }
					);
				} else {
					auto buffer = renderGraph.GetBufferByRGResource(br.Resource)->As<VulkanSharedStorageBuffer>();
					vkBatch.PreBatchBarrier.BufferBarriers.push_back(
						CreateSameQueueBufferBarrier(family, buffer, br.SrcAccess, br.DstAccess)
					);
				}
			}

			// Incoming = acquire on destination queue
			for (const auto& tr : rgBatch.IncomingInterQueueTransitions) {
				if (tr.ResourceType == RenderGraphResourceType::Image) {
					auto image = renderGraph.GetImageByRGResource(tr.Resource)->As<VulkanImage>();
					vkBatch.PreBatchBarrier.ImageBarriers.push_back(
						VulkanImageMemoryBarrier{ image, CreateAcquireImageBarrier(tr.SrcQueue, tr.DstQueue, image, tr.SrcAccess, tr.DstAccess) }
					);
				} else {
					auto buffer = renderGraph.GetBufferByRGResource(tr.Resource)->As<VulkanSharedStorageBuffer>();
					vkBatch.PreBatchBarrier.BufferBarriers.push_back(
						CreateAcquireBufferBarrier(tr.SrcQueue, tr.DstQueue, buffer, tr.DstAccess)
					);
				}
			}

			// Outgoing = release on source queue
			for (const auto& tr : rgBatch.OutgoingInterQueueTransitions) {
				if (tr.ResourceType == RenderGraphResourceType::Image) {
					auto image = renderGraph.GetImageByRGResource(tr.Resource)->As<VulkanImage>();
					vkBatch.PostBatchBarrier.ImageBarriers.push_back(
						VulkanImageMemoryBarrier{ image, CreateReleaseImageBarrier(tr.SrcQueue, tr.DstQueue, image, tr.SrcAccess, tr.DstAccess) }
					);
				} else {
					auto buffer = renderGraph.GetBufferByRGResource(tr.Resource)->As<VulkanSharedStorageBuffer>();
					vkBatch.PostBatchBarrier.BufferBarriers.push_back(
						CreateReleaseBufferBarrier(tr.SrcQueue, tr.DstQueue, buffer, tr.SrcAccess)
					);
				}
			}

			result.push_back(std::move(batch));
		}

		return result;
	}
}
