#include "lypch.h"
#include "VulkanRenderer.h"
#include "Renderer.h"

#include "Semaphore.h"

#include "ExecutionBatch.h"

#include "Context/VulkanSwapChain.h"
#include "Context/VulkanContext.h"

#include "Device/VulkanRenderDevice.h"
#include "Device/RenderDeviceScene.h"

#include "RenderGraph/RenderGraphCompiler.h"

#include "Memory/Buffer/Buffer.h"
#include "Commands/VulkanCommandPool.h"

#include "Events/InputEvent.h"

namespace Lucy {
	
	VulkanRenderer::VulkanRenderer(RendererConfiguration config, const Ref<Window>& window)
		: RendererBackend(config, window) {
	}

	void VulkanRenderer::Init() {
		const auto& vulkanContext = m_Context->As<VulkanContext>();
		const auto& vulkanDevice = m_RenderDevice->As<VulkanRenderDevice>();

		vulkanContext->Init();
		vulkanContext->PrintInfo();

		vulkanDevice->Init(vulkanContext->GetVulkanInstance(), vulkanContext->GetValidationLayers(),
			vulkanContext->GetWindow()->GetVulkanSurface(), VulkanContext::GetAPIVersion());

		m_SwapChain->Init();

		m_RenderCommandQueue->Init();

		const size_t swapImageCount = m_SwapChain->As<VulkanSwapChain>()->GetSwapChainImageCount();

		m_InFlightFences.reserve(m_MaxFramesInFlight);
		m_ImageAvailableSemaphores.reserve(m_MaxFramesInFlight);
		m_SceneFinishedSemaphores.reserve(m_MaxFramesInFlight);

		m_RenderFinishedSemaphores.reserve(swapImageCount);

		m_BridgeSemaphores.resize(m_MaxFramesInFlight);
		m_FrameFenceValues.resize(m_MaxFramesInFlight, 0);

		for (size_t i = 0; i < m_MaxFramesInFlight; i++) {
			m_InFlightFences.emplace_back(SemaphoreType::Timeline, vulkanDevice);
			m_ImageAvailableSemaphores.emplace_back(SemaphoreType::Binary, vulkanDevice);
			m_SceneFinishedSemaphores.emplace_back(SemaphoreType::Binary, vulkanDevice);
		}

		for (size_t i = 0; i < swapImageCount; i++) {
			m_RenderFinishedSemaphores.emplace_back(SemaphoreType::Binary, vulkanDevice);
		}

		m_TransientCommandPool = Memory::CreateUnique<VulkanTransientCommandPool>(vulkanDevice);

		//for imgui
		m_ImGuiRenderCommandList = Memory::CreateUnique<RenderCommandList>(RenderCommandListCreateInfo{
			.RenderDevice = vulkanDevice,
			.TargetQueueFamily = TargetQueueFamily::Graphics
		});
	}

	void VulkanRenderer::BeginFrame() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::BeginFrame");
		using enum RenderContextResultCodes;

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		VkDevice deviceVulkanHandle = renderDevice->GetLogicalDevice();

		{
			LUCY_PROFILE_NEW_EVENT("VulkanRenderer::BeginFrame::TimelineWait");

			const uint64_t frameValue = m_FrameFenceValues[m_CurrentFrameIndex];
			m_InFlightFences[m_CurrentFrameIndex].Wait(frameValue);

			if (frameValue > 0)
				ProcessQueryResults();

			renderDevice->ResetPipelineQuery(m_CurrentFrameIndex);
			renderDevice->ResetTimestampQuery(m_CurrentFrameIndex);
			
			m_RenderCommandQueue->ResetFrameSlotRecordersIfCompleted(m_CurrentFrameIndex, TargetQueueFamily::Graphics);
			m_RenderCommandQueue->ResetFrameSlotRecordersIfCompleted(m_CurrentFrameIndex, TargetQueueFamily::Compute);
			m_RenderCommandQueue->ResetFrameSlotRecordersIfCompleted(m_CurrentFrameIndex, TargetQueueFamily::Transfer);

			m_ImGuiRenderCommandList->ResetRenderCommand(m_CurrentFrameIndex);
		}

		{
			LUCY_PROFILE_NEW_EVENT("VulkanRenderer::BeginFrame::AcquireNextImage");
			
			const auto& swapChain = GetSwapChain()->As<VulkanSwapChain>();
			m_LastSwapChainResult = swapChain->AcquireNextImage(&m_ImageAvailableSemaphores[m_CurrentFrameIndex], m_ImageIndex);
			if (m_LastSwapChainResult == ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == SUBOPTIMAL_KHR || m_LastSwapChainResult == NOT_READY)
				return;
		}
	}

	void VulkanRenderer::RenderFrame() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::RenderFrame");
		using enum RenderContextResultCodes;

		if (m_LastSwapChainResult == ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == SUBOPTIMAL_KHR || m_LastSwapChainResult == NOT_READY)
			return;

		const auto& vulkanDevice = m_RenderDevice->As<VulkanRenderDevice>();
		auto& submitQueue = m_RenderCommandQueue->GetRenderSubmitQueue();
		const bool hasSceneWork = !submitQueue.empty();

		uint64_t signalValue = m_FrameFenceValues[m_CurrentFrameIndex] + 1;

		if (hasSceneWork) {
			LUCY_PROFILE_NEW_EVENT("VulkanRenderer::RenderFrame::SubmitQueue");

			m_RenderCommandQueue->AllocateCommandLists(submitQueue);
			LinkBatches(submitQueue, signalValue);

			for (auto& [id, info] : submitQueue) {
				auto& vkBatch = info.Batch.AsVulkanBatch();

				RenderCommandList& cmdList = m_RenderCommandQueue->GetNextAvailableCommandList(m_CurrentFrameIndex, vkBatch.QueueFamily);
				const auto& primaryCommandPool = cmdList.GetPrimaryCommandPool();
				
				vulkanDevice->BeginCommandBuffer(primaryCommandPool);

				if (!vkBatch.PreBatchBarrier.ImageBarriers.empty() || !vkBatch.PreBatchBarrier.BufferBarriers.empty()) {
					VkCommandBuffer cmdBuffer = static_cast<VkCommandBuffer>(primaryCommandPool->GetCommandBuffer(m_CurrentFrameIndex));
					vulkanDevice->BeginDebugMarker(cmdBuffer, "PreBatchVulkanBarrier");
					ExecuteVulkanBatchBarrier(cmdBuffer, vkBatch.PreBatchBarrier);
					vulkanDevice->EndDebugMarker(cmdBuffer);
				}

				for (size_t i = 0; i < info.SubmitFuncs.size(); i++) {
					RenderGraphPass* pass = vkBatch.Passes[i];
					/*auto it = std::ranges::find_if(vkBatch.PassBarriers, [&](const VulkanPassBarrier& passBarrier) {
						return passBarrier.Pass == pass;
					});*/

					/*if (it != vkBatch.PassBarriers.end()) {
						VkCommandBuffer cmdBuffer = static_cast<VkCommandBuffer>(primaryCommandPool->GetCommandBuffer(m_CurrentFrameIndex));
						vulkanDevice->BeginDebugMarker(cmdBuffer, "PassVulkanBarrier");
						ExecuteVulkanBatchBarrier(cmdBuffer, it->Barrier);
						vulkanDevice->EndDebugMarker(cmdBuffer);
					}*/

					info.SubmitFuncs[i](cmdList);
				}

				if (!vkBatch.PostBatchBarrier.ImageBarriers.empty() || !vkBatch.PostBatchBarrier.BufferBarriers.empty()) {
					VkCommandBuffer cmdBuffer = static_cast<VkCommandBuffer>(primaryCommandPool->GetCommandBuffer(m_CurrentFrameIndex));
					vulkanDevice->BeginDebugMarker(cmdBuffer, "PostBatchVulkanBarrier");
					ExecuteVulkanBatchBarrier(cmdBuffer, vkBatch.PostBatchBarrier);
					vulkanDevice->EndDebugMarker(cmdBuffer);
				}

				vulkanDevice->EndCommandBuffer(primaryCommandPool);
				vulkanDevice->SubmitWorkToGPUAsBatch(cmdList, info.Batch);
			}
		}

		InternalImGuiPass(signalValue, hasSceneWork);

		m_FrameFenceValues[m_CurrentFrameIndex] = signalValue;
	}

	void VulkanRenderer::EndFrame() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::EndFrame");
		using enum RenderContextResultCodes;
		if (m_LastSwapChainResult == ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == SUBOPTIMAL_KHR || m_LastSwapChainResult == NOT_READY)
			return;

		const auto& swapChain = GetSwapChain()->As<VulkanSwapChain>();
		m_LastSwapChainResult = swapChain->Present(&m_RenderFinishedSemaphores[m_ImageIndex], m_ImageIndex);
	}

	void VulkanRenderer::FlushDeletionQueue() {
		auto& deletionQueue = m_ResourceDeletionQueues[m_CurrentFrameIndex];

		if (deletionQueue.empty())
			return;

		const uint64_t frameValue = m_FrameFenceValues[m_CurrentFrameIndex];
		m_InFlightFences[m_CurrentFrameIndex].Wait(frameValue);

		for (auto it = deletionQueue.rbegin(); it != deletionQueue.rend(); ++it)
			(*it)(m_RenderDevice);

		deletionQueue.clear();
	}

	void VulkanRenderer::ProcessQueryResults() {
		m_CommandQueueMetricsOutput.Time = 0;
		m_CommandQueueMetricsOutput.TimeOfPasses.clear();

		const auto& vulkanDevice = m_RenderDevice->As<VulkanRenderDevice>();
		auto timestampResults = vulkanDevice->GetQueryResults(RenderDeviceQueryType::Timestamp, m_CurrentFrameIndex);
		auto pipelineResults = vulkanDevice->GetQueryResults(RenderDeviceQueryType::Pipeline, m_CurrentFrameIndex);

		const auto ProcessQuery = [](const RenderCommandListQueryData& queryData, const std::vector<uint64_t>& results, double timestampPeriod) -> RenderCommandQueueMetricsOutput {
			RenderCommandQueueMetricsOutput metricsOutput{};
			metricsOutput.Time = 0.0;
			for (const auto& scope : queryData.TimestampScopes) {
				uint64_t begin = results[scope.BeginQueryIndex];
				uint64_t end = results[scope.EndQueryIndex];
				float ms = static_cast<float>(end - begin) * timestampPeriod / 1000000.0f;
				metricsOutput.TimeOfPasses[scope.PassName] = ms;
				metricsOutput.Time += ms;
			}
			return metricsOutput;
		};

		const auto& cmdListsGraphics = m_RenderCommandQueue->GetCommandLists(TargetQueueFamily::Graphics);
		const auto& cmdListsCompute = m_RenderCommandQueue->GetCommandLists(TargetQueueFamily::Compute);

		for (const auto& cmdList : cmdListsGraphics) {
			const auto& queryData = cmdList.GetQueryData(m_CurrentFrameIndex);
			if (!queryData.TimestampScopes.empty()) {
				auto metricsOutput = ProcessQuery(queryData, timestampResults, vulkanDevice->GetTimestampPeriod());
				m_CommandQueueMetricsOutput.Time += metricsOutput.Time;
				for (const auto& [passName, time] : metricsOutput.TimeOfPasses)
					m_CommandQueueMetricsOutput.TimeOfPasses[passName] += time;
			}
		}

		for (const auto& cmdList : cmdListsCompute) {
			const auto& queryData = cmdList.GetQueryData(m_CurrentFrameIndex);
			if (!queryData.TimestampScopes.empty()) {
				auto metricsOutput = ProcessQuery(queryData, timestampResults, vulkanDevice->GetTimestampPeriod());
				m_CommandQueueMetricsOutput.Time += metricsOutput.Time;
				for (const auto& [passName, time] : metricsOutput.TimeOfPasses)
					m_CommandQueueMetricsOutput.TimeOfPasses[passName] += time;
			}
		}


	}

	void VulkanRenderer::InternalImGuiPass(uint64_t signalValue, bool hasSceneWork) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::InternalImGuiPass");
		const auto& imguiPool = m_ImGuiRenderCommandList->GetPrimaryCommandPool();
		const auto& swapChain = GetSwapChain()->As<VulkanSwapChain>();

		m_RenderDevice->BeginCommandBuffer(imguiPool);
		m_ImGuiPassImpl.Render(swapChain, *m_ImGuiRenderCommandList.get());
		m_RenderDevice->EndCommandBuffer(imguiPool);

		VulkanSemaphore& waitSem = hasSceneWork
			? m_SceneFinishedSemaphores[m_CurrentFrameIndex]
			: m_ImageAvailableSemaphores[m_CurrentFrameIndex];

		m_RenderDevice->As<VulkanRenderDevice>()->SubmitWorkToGPU(*m_ImGuiRenderCommandList.get(), waitSem, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			m_RenderFinishedSemaphores[m_ImageIndex], m_InFlightFences[m_CurrentFrameIndex], signalValue);
	}

	void VulkanRenderer::LinkBatches(RenderSubmitQueue& submitQueue, uint64_t signalValue) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::LinkBatches");

		for (auto& [id, info] : submitQueue) {
			auto& vkBatch = info.Batch.AsVulkanBatch();
			vkBatch.Waits.clear();
			vkBatch.Signals.clear();
		}

		if (submitQueue.empty())
			return;

		auto& imageAvailable = m_ImageAvailableSemaphores[m_CurrentFrameIndex];
		auto& sceneFinishedSemaphore = m_SceneFinishedSemaphores[m_CurrentFrameIndex];

		while (m_BridgeSemaphores[m_CurrentFrameIndex].size() < (submitQueue.size() > 0 ? submitQueue.size() - 1 : 0)) {
			m_BridgeSemaphores[m_CurrentFrameIndex].emplace_back(SemaphoreType::Binary, m_RenderDevice);
		}

		const auto GetDefaultWaitStage = [](TargetQueueFamily family) -> VkPipelineStageFlags2 {
			switch (family) {
				case TargetQueueFamily::Graphics: return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
				case TargetQueueFamily::Compute:  return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
				case TargetQueueFamily::Transfer: return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				default:                          
					LUCY_ASSERT(false);
					return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			}
		};

		const auto GetDefaultSignalStage = [](TargetQueueFamily family) -> VkPipelineStageFlags2 {
			switch (family) {
				case TargetQueueFamily::Graphics: return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
				case TargetQueueFamily::Compute:  return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
				case TargetQueueFamily::Transfer: return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				default:                          
					LUCY_ASSERT(false);
					return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			}
		};

		// First batch waits on the acquire semaphore for this frame slot.
		auto firstIt = submitQueue.begin();
		auto& firstBatch = firstIt->second.Batch.AsVulkanBatch();
		firstBatch.Waits.emplace_back(GetDefaultWaitStage(firstBatch.QueueFamily), 0, imageAvailable);

		// Adjacent batch bridges.
		size_t bridgeIndex = 0;
		for (auto it = submitQueue.begin(); it != submitQueue.end(); ) {
			auto next = std::next(it);
			if (next == submitQueue.end())
				break;

			auto& srcBatch = it->second.Batch.AsVulkanBatch();
			auto& dstBatch = next->second.Batch.AsVulkanBatch();
			auto& bridge = m_BridgeSemaphores[m_CurrentFrameIndex][bridgeIndex++];

			srcBatch.Signals.emplace_back(GetDefaultSignalStage(srcBatch.QueueFamily), 0, bridge);
			dstBatch.Waits.emplace_back(GetDefaultWaitStage(dstBatch.QueueFamily), 0, bridge);

			it = next;
		}

		auto lastIt = std::prev(submitQueue.end());
		auto& lastBatch = lastIt->second.Batch.AsVulkanBatch();
		const VkPipelineStageFlags2 lastSignalStages = GetDefaultSignalStage(lastBatch.QueueFamily);

		lastBatch.Signals.emplace_back(lastSignalStages, 0, sceneFinishedSemaphore);
	}

	void VulkanRenderer::ExecuteVulkanBatchBarrier(VkCommandBuffer cmdBuffer, const VulkanBatchBarrier& barrier) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::RenderFrame::SubmitQueue::ExecuteVulkanBatchBarrier");

		std::vector<VkImageMemoryBarrier2> imageBarriers;
		imageBarriers.reserve(barrier.ImageBarriers.size());
		for (const auto& imageBarrier : barrier.ImageBarriers) {
			auto vkBarrier = imageBarrier.Barrier;
			vkBarrier.image = imageBarrier.Image->GetVulkanHandle();
			imageBarriers.emplace_back(vkBarrier);
		}

		VkDependencyInfo depInfo{};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size());
		depInfo.pImageMemoryBarriers = imageBarriers.empty() ? nullptr : imageBarriers.data();
		depInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(barrier.BufferBarriers.size());
		depInfo.pBufferMemoryBarriers = barrier.BufferBarriers.empty() ? nullptr : barrier.BufferBarriers.data();

		vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

		for (const auto& [image, barrier] : barrier.ImageBarriers)
			image->SetLayout(barrier.newLayout);
	}
	
	void VulkanRenderer::SubmitBatchesToRender(std::vector<ExecutionBatch>& batches, const std::unordered_map<std::string, RenderFrameHandles>& renderFrameHandleMap) {
		//LUCY_ASSERT(!Renderer::IsOnRenderThread(), "SubmitBatchesToRender should only be called on the main thread!");
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::SubmitBatchesToRender");

		for (const auto& batch : batches) {
			const VulkanExecutionBatch& vkBatch = batch.AsVulkanBatch();
			
			std::vector<RenderSubmitFunc> passExecuteFuncs;
			passExecuteFuncs.reserve(vkBatch.Passes.size());

			for (const auto& pass : vkBatch.Passes) {
				auto queueFamily = pass->GetTargetQueueFamily();

				switch (queueFamily) {
					case TargetQueueFamily::Compute: {
						passExecuteFuncs.push_back([=](RenderCommandList& cmdList) {
							LUCY_PROFILE_NEW_EVENT("RendererBackend::SubmitToCompute");
							pass->Execute(cmdList);
						});
						break;
					}
					case TargetQueueFamily::Graphics: {
						if (!renderFrameHandleMap.contains(pass->GetName())) {
							passExecuteFuncs.push_back([=](RenderCommandList& cmdList) {
								LUCY_PROFILE_NEW_EVENT("RendererBackend::SubmitToCompute");
								pass->Execute(cmdList);
							});
							break;
						}
						const auto& [renderPassHandle, frameBufferHandle] = renderFrameHandleMap.at(pass->GetName());
						passExecuteFuncs.push_back([=](RenderCommandList& cmdList) {
							LUCY_PROFILE_NEW_EVENT("RendererBackend::SubmitToRender");
							const auto& device = GetRenderDevice();
							const auto& renderPass = device->AccessResource<RenderPass>(renderPassHandle);
							const auto& frameBuffer = device->AccessResource<FrameBuffer>(frameBufferHandle);
							device->BeginRenderPass(renderPass, frameBuffer, cmdList.GetPrimaryCommandPool());
							pass->Execute(cmdList);
							device->EndRenderPass(renderPass);
						});
						break;
					}
					case TargetQueueFamily::Transfer: {
						passExecuteFuncs.push_back([=](RenderCommandList& cmdList) {
							LUCY_PROFILE_NEW_EVENT("RendererBackend::SubmitToTransfer");
							pass->Execute(cmdList);
						});
						break;
					}
					default:
						LUCY_ASSERT(false);
						break;
				}
			}
			EnqueueToRenderCommandQueue(batch, passExecuteFuncs);
		}
	}

	RenderContextResultCodes VulkanRenderer::WaitAndPresent() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::WaitAndPresent");
		
		BeginFrame();
		FlushDeletionQueue();
		FlushCommandQueue();

		m_RenderDevice->GetScene()->SyncFrame(m_CurrentFrameIndex);
		m_RenderDevice->As<VulkanRenderDevice>()->GetUploadManager()->SyncFrame(m_CurrentFrameIndex);

		RenderFrame();

		EndFrame();

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;

		m_RenderCommandQueue->ClearSubmitQueue();

		return (RenderContextResultCodes)m_LastSwapChainResult;
	}

	void VulkanRenderer::Destroy() {
		auto& allocator = GetRenderDevice()->As<VulkanRenderDevice>()->GetAllocator();
		allocator.DestroyBuffer(s_IDBuffer, s_IDBufferVma);

		m_TransientCommandPool->Destroy();

		const uint32_t swapImageCount = m_SwapChain->As<VulkanSwapChain>()->GetSwapChainImageCount();

		const auto& swapChain = GetSwapChain();
		swapChain->Destroy();

		FlushDeletionQueue();

		for (uint32_t i = 0; i < m_MaxFramesInFlight; i++) {
			m_ImageAvailableSemaphores[i].Destroy();
			m_InFlightFences[i].Destroy();
		}

		for (uint32_t i = 0; i < swapImageCount; i++) {
			m_RenderFinishedSemaphores[i].Destroy();
		}

		for (auto& perFrameBridges : m_BridgeSemaphores) {
			for (auto& semaphore : perFrameBridges)
				semaphore.Destroy();
		}

		FlushCommandQueue();
		RendererBackend::Destroy();
	}

	// Should not be used in a loop 
	void VulkanRenderer::SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::SubmitImmediateCommand");
		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		renderDevice->SubmitImmediateCommand(func, m_TransientCommandPool);
	}

	void VulkanRenderer::OnWindowResize() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::OnWindowResize");

		const auto& swapChain = GetSwapChain();
		swapChain->Recreate();

		//RecreateCommandQueue();

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		auto& allocator = renderDevice->GetAllocator();
		allocator.DestroyBuffer(s_IDBuffer, s_IDBufferVma);

		s_IDBuffer = VK_NULL_HANDLE;
		s_IDBufferVma = VK_NULL_HANDLE;
	}

	void VulkanRenderer::OnViewportResize() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::OnViewportResize");

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		auto& allocator = renderDevice->GetAllocator();
		allocator.DestroyBuffer(s_IDBuffer, s_IDBufferVma);

		s_IDBuffer = VK_NULL_HANDLE;	
		s_IDBufferVma = VK_NULL_HANDLE;
	}
	
	glm::vec3 VulkanRenderer::OnMousePicking(const EntityPickedEvent& e, const Ref<Image>& currentFrameBufferImage) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::OnMousePicking");

		float viewportMouseX = e.GetViewportMouseX();
		float viewportMouseY = e.GetViewportMouseY();

		if (viewportMouseX < 0 && viewportMouseY < 0)
			return glm::vec3(-1.0f);

		const auto& image = currentFrameBufferImage->As<VulkanImage2D>();
		uint32_t imageWidth = image->GetWidth();
		uint32_t imageHeight = image->GetHeight();
		VkDeviceSize imageSize = (VkDeviceSize)imageWidth * imageHeight * 4;

		auto& allocator = GetRenderDevice()->As<VulkanRenderDevice>()->GetAllocator();
		if (!s_IDBuffer)
			allocator.CreateVulkanBufferVma(MemoryUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, false, s_IDBuffer, s_IDBufferVma);

		image->SetLayoutImmediate(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		image->CopyImageToBufferImmediate(s_IDBuffer);

		ByteBuffer rawData;

		void* rawDataMapped;
		allocator.MapMemory(s_IDBufferVma, rawDataMapped);
		rawData.SetData((uint8_t*)rawDataMapped, imageSize);
		allocator.UnmapMemory(s_IDBufferVma);

		uint32_t flippedY = imageHeight - 1 - (uint32_t)(viewportMouseY);
		uint32_t bufferPos = (uint32_t) (4 * ((flippedY * imageWidth) + viewportMouseX));
		glm::vec3 meshID = glm::vec3(rawData[bufferPos], rawData[bufferPos + 1], rawData[bufferPos + 2]);

		image->SetLayoutImmediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//checking if the data that is being read make sense
		if ((meshID.x > 255.0f || meshID.x < 0.0f) ||
			(meshID.y > 255.0f || meshID.y < 0.0f) ||
			(meshID.z > 255.0f || meshID.z < 0.0f))
			return glm::vec3(-1.0f);

		//checking if we clicked on the void
		if (meshID.x == 0 && meshID.y == 0 && meshID.z == 0)
			return glm::vec3(-1.0f);

		return meshID;
	}

	void VulkanRenderer::InitializeImGui() {
		m_ImGuiPassImpl.Init(this);
	}
}