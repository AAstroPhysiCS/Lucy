#include "lypch.h"
#include "VulkanRenderer.h"

#include "Renderer/Synchronization/VulkanSyncItems.h"

#include "Context/VulkanSwapChain.h"
#include "Context/VulkanContext.h"

#include "Device/VulkanRenderDevice.h"

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
		m_RenderComputeCommandQueue->Init();

		m_WaitSemaphores.reserve(m_MaxFramesInFlight);
		m_SignalSemaphores.reserve(m_MaxFramesInFlight);
		m_InFlightFences.reserve(m_MaxFramesInFlight);

		m_WaitSemaphoresCompute.reserve(m_MaxFramesInFlight);
		m_SignalSemaphoresCompute.reserve(m_MaxFramesInFlight);
		m_InFlightFencesCompute.reserve(m_MaxFramesInFlight);

		for (size_t i = 0; i < m_MaxFramesInFlight; i++) {
			m_WaitSemaphores.emplace_back(vulkanDevice);
			m_SignalSemaphores.emplace_back(vulkanDevice);
			m_InFlightFences.emplace_back(vulkanDevice);

			m_WaitSemaphoresCompute.emplace_back(vulkanDevice);
			m_SignalSemaphoresCompute.emplace_back(vulkanDevice);
			m_InFlightFencesCompute.emplace_back(vulkanDevice);
		}

		m_TransientCommandPool = Memory::CreateRef<VulkanTransientCommandPool>(vulkanDevice);
	}

	void VulkanRenderer::BeginFrame() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::BeginFrame");
		using enum RenderContextResultCodes;

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		VkDevice deviceVulkanHandle = renderDevice->GetLogicalDevice();

		vkWaitForFences(deviceVulkanHandle, 1, &m_InFlightFences[m_CurrentFrameIndex].GetFence(), VK_TRUE, UINT64_MAX);
		vkResetFences(deviceVulkanHandle, 1, &m_InFlightFences[m_CurrentFrameIndex].GetFence());

		if (!m_RenderComputeCommandQueue->IsEmpty()) {
			vkWaitForFences(deviceVulkanHandle, 1, &m_InFlightFencesCompute[m_CurrentFrameIndex].GetFence(), VK_TRUE, UINT64_MAX);
			vkResetFences(deviceVulkanHandle, 1, &m_InFlightFencesCompute[m_CurrentFrameIndex].GetFence());
		}

		m_UseComputeSemaphore = false;

		const auto& swapChain = GetSwapChain()->As<VulkanSwapChain>();
		m_LastSwapChainResult = swapChain->AcquireNextImage(m_WaitSemaphores[m_CurrentFrameIndex], m_ImageIndex);
		if (m_LastSwapChainResult == ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == SUBOPTIMAL_KHR || m_LastSwapChainResult == NOT_READY)
			return;
	}
	
	void VulkanRenderer::RenderFrame() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::RenderFrame");
		using enum RenderContextResultCodes;
		if (m_LastSwapChainResult == ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == SUBOPTIMAL_KHR || m_LastSwapChainResult == NOT_READY)
			return;

		Fence& currentFrameFence = m_InFlightFences[m_CurrentFrameIndex];
		Semaphore& currentFrameWaitSemaphore = m_WaitSemaphores[m_CurrentFrameIndex];
		Semaphore& currentFrameSignalSemaphore = m_SignalSemaphores[m_CurrentFrameIndex];

		Fence& currentFrameFenceCompute = m_InFlightFencesCompute[m_CurrentFrameIndex];
		Semaphore& currentFrameWaitSemaphoreCompute = m_WaitSemaphoresCompute[m_CurrentFrameIndex];
		Semaphore& currentFrameSignalSemaphoreCompute = m_SignalSemaphoresCompute[m_CurrentFrameIndex];

		const auto& graphicsCmdLists = m_RenderCommandQueue->GetCommandLists();
		const auto& computeCmdLists = m_RenderComputeCommandQueue->GetCommandLists();

		std::vector<Ref<CommandPool>> graphicsCmdPools;
		graphicsCmdPools.reserve(graphicsCmdLists.size());

		for (const auto& cmdList : graphicsCmdLists) {
			if (!cmdList)
				graphicsCmdPools.emplace_back(cmdList.GetPrimaryCommandPool());
		}

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		renderDevice->SubmitWorkToGPU(TargetQueueFamily::Graphics, graphicsCmdPools, &currentFrameFence, &currentFrameWaitSemaphore, &currentFrameSignalSemaphore);

		if (!m_RenderComputeCommandQueue->IsEmpty()) {
			std::vector<Ref<CommandPool>> computeCmdPools;
			computeCmdPools.reserve(computeCmdLists.size());

			for (const auto& cmdList : computeCmdLists) {
				if (!cmdList)
					computeCmdPools.emplace_back(cmdList.GetPrimaryCommandPool());
			}

			// Submission to GPU for compute work (work is pending)
			if (renderDevice->SubmitWorkToGPU(TargetQueueFamily::Compute, computeCmdPools, &currentFrameFenceCompute, &currentFrameSignalSemaphore, &currentFrameSignalSemaphoreCompute)) {
				m_UseComputeSemaphore = true; // Flag to use compute semaphore for presentation
			} else {
				m_UseComputeSemaphore = false; // Fall back to graphics semaphore
			}
		}
	}

	void VulkanRenderer::EndFrame() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::EndFrame");
		using enum RenderContextResultCodes;
		if (m_LastSwapChainResult == ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == SUBOPTIMAL_KHR || m_LastSwapChainResult == NOT_READY)
			return;

		const auto& swapChain = GetSwapChain()->As<VulkanSwapChain>();
		Semaphore& presentSemaphore = m_UseComputeSemaphore ? m_SignalSemaphoresCompute[m_CurrentFrameIndex] : m_SignalSemaphores[m_CurrentFrameIndex];
		m_LastSwapChainResult = swapChain->Present(presentSemaphore, m_ImageIndex);
	}

	void VulkanRenderer::FlushDeletionQueue() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::FlushDeletionQueue");
		auto& currentDeletionQueue = m_ResourceDeletionQueues[m_CurrentFrameIndex];
		if (currentDeletionQueue.empty())
			return;

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		auto result = vkGetFenceStatus(renderDevice->GetLogicalDevice(), m_InFlightFences[m_CurrentFrameIndex].GetFence());
		if (result != VK_SUCCESS)
			vkWaitForFences(renderDevice->GetLogicalDevice(), 1, &m_InFlightFences[m_CurrentFrameIndex].GetFence(), VK_FALSE, UINT64_MAX);

		if (!m_RenderComputeCommandQueue->IsEmpty()) {
			auto resultCompute = vkGetFenceStatus(renderDevice->GetLogicalDevice(), m_InFlightFencesCompute[m_CurrentFrameIndex].GetFence());

			if (resultCompute != VK_SUCCESS)
				vkWaitForFences(renderDevice->GetLogicalDevice(), 1, &m_InFlightFencesCompute[m_CurrentFrameIndex].GetFence(), VK_FALSE, UINT64_MAX);
		}
		
		size_t oldDeletionQueueSize = currentDeletionQueue.size();
		for (const auto& deletionFunc : currentDeletionQueue)
			deletionFunc();
		currentDeletionQueue.erase(currentDeletionQueue.begin(), currentDeletionQueue.begin() + oldDeletionQueueSize);
	}
	
	RenderContextResultCodes VulkanRenderer::WaitAndPresent() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::WaitAndPresent");
		
		BeginFrame();
		FlushCommandQueue();
		FlushSubmitQueue();
		RenderFrame();
		EndFrame();
		FlushDeletionQueue();

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;

		return (RenderContextResultCodes)m_LastSwapChainResult;
	}

	void VulkanRenderer::ExecuteBarrier(void* commandBufferHandle, Ref<Image> image) {
		const auto& vulkanImage = image->As<VulkanImage>();
		ExecuteBarrier(commandBufferHandle, (void*)vulkanImage->GetVulkanHandle(), vulkanImage->GetCurrentLayout(), vulkanImage->GetLayerCount(), vulkanImage->GetMaxMipLevel());
	}

	void VulkanRenderer::ExecuteBarrier(void* commandBufferHandle, void* imageHandle, uint32_t imageLayout, uint32_t layerCount, uint32_t mipCount) {
		VkImageSubresourceRange subResourceRange = VulkanAPI::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, mipCount, layerCount);

		ImageMemoryBarrierCreateInfo createInfo;
		createInfo.ImageHandle = (VkImage)imageHandle;
		createInfo.NewLayout = (VkImageLayout)imageLayout;
		createInfo.OldLayout = createInfo.NewLayout; //no layout changes
		createInfo.SubResourceRange = subResourceRange;

		ImageMemoryBarrier barrier(createInfo);

		barrier.RunBarrier((VkCommandBuffer)commandBufferHandle);
	}

	void VulkanRenderer::Destroy() {
		auto& allocator = GetRenderDevice()->As<VulkanRenderDevice>()->GetAllocator();
		allocator.DestroyBuffer(s_IDBuffer, s_IDBufferVma);

		m_TransientCommandPool->Destroy();

		const auto& swapChain = GetSwapChain();
		swapChain->Destroy();

		FlushDeletionQueue();

		for (uint32_t i = 0; i < m_MaxFramesInFlight; i++) {
			m_WaitSemaphores[i].Destroy(m_RenderDevice);
			m_SignalSemaphores[i].Destroy(m_RenderDevice);
			m_InFlightFences[i].Destroy(m_RenderDevice);

			m_WaitSemaphoresCompute[i].Destroy(m_RenderDevice);
			m_SignalSemaphoresCompute[i].Destroy(m_RenderDevice);
			m_InFlightFencesCompute[i].Destroy(m_RenderDevice);
		}

		FlushCommandQueue();
		RendererBackend::Destroy();
	}

	// Should not be used in a loop 
	void VulkanRenderer::RTDirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		renderDevice->SubmitImmediateCommand([&](VkCommandBuffer commandBuffer) {
			VkBufferCopy copyRegion = VulkanAPI::BufferCopy(0, 0, size);
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);
		}, m_TransientCommandPool);
	}

	// Should not be used in a loop 
	void VulkanRenderer::SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func) {
		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		renderDevice->SubmitImmediateCommand(func, m_TransientCommandPool);
	}

	void VulkanRenderer::OnWindowResize() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::OnWindowResize");

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		renderDevice->WaitForDevice();

		const auto& swapChain = GetSwapChain();
		swapChain->Recreate();

		RecreateCommandQueue();

		auto& allocator = renderDevice->GetAllocator();
		allocator.DestroyBuffer(s_IDBuffer, s_IDBufferVma);

		s_IDBuffer = VK_NULL_HANDLE;
		s_IDBufferVma = VK_NULL_HANDLE;
	}

	void VulkanRenderer::OnViewportResize() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::OnViewportResize");

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		renderDevice->WaitForDevice();

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
			allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, s_IDBuffer, s_IDBufferVma);

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
		m_ImGuiPass.Init(this);
	}

	void VulkanRenderer::RTRenderImGui() {
		auto swapChain = GetSwapChain()->As<VulkanSwapChain>();
		EnqueueToRenderCommandQueue([this, swapChain](RenderCommandList& cmdList) {
			m_ImGuiPass.Render(swapChain, cmdList);
		});
	}
}