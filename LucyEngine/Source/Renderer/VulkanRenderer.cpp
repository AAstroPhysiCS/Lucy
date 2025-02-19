#include "lypch.h"
#include "VulkanRenderer.h"

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
		RendererBackend::Init();

		m_WaitSemaphores.resize(m_MaxFramesInFlight);
		m_SignalSemaphores.resize(m_MaxFramesInFlight);
		m_InFlightFences.resize(m_MaxFramesInFlight);

		m_TransientCommandPool = Memory::CreateRef<VulkanTransientCommandPool>(GetRenderDevice()->As<VulkanRenderDevice>());
	}

	void VulkanRenderer::BeginFrame() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::BeginFrame");

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		VkDevice deviceVulkanHandle = renderDevice->GetLogicalDevice();

		vkWaitForFences(deviceVulkanHandle, 1, &m_InFlightFences[m_CurrentFrameIndex].GetFence(), VK_TRUE, UINT64_MAX);
		vkResetFences(deviceVulkanHandle, 1, &m_InFlightFences[m_CurrentFrameIndex].GetFence());

		m_LastSwapChainResult = GetRenderContext()->As<VulkanContext>()->GetSwapChain().AcquireNextImage(m_WaitSemaphores[m_CurrentFrameIndex].GetSemaphore(), m_ImageIndex);
		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR || m_LastSwapChainResult == VK_NOT_READY)
			return;
	}
	
	void VulkanRenderer::RenderFrame() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::RenderFrame");
		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR || m_LastSwapChainResult == VK_NOT_READY)
			return;

		Fence currentFrameFence = m_InFlightFences[m_CurrentFrameIndex];
		Semaphore currentFrameWaitSemaphore = m_WaitSemaphores[m_CurrentFrameIndex];
		Semaphore currentFrameSignalSemaphore = m_SignalSemaphores[m_CurrentFrameIndex];

		const auto& cmdLists = GetCommandLists();

		std::vector<Ref<CommandPool>> cmdPools;
		cmdPools.resize(cmdLists.size(), nullptr);

		const auto& renderDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		for (size_t i = 0; const auto& cmdList : cmdLists)
			cmdPools[i++] = cmdList.GetPrimaryCommandPool();
			
		renderDevice->SubmitWorkToGPU(TargetQueueFamily::Graphics, cmdPools, currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);
	}

	void VulkanRenderer::EndFrame() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::EndFrame");
		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR || m_LastSwapChainResult == VK_NOT_READY)
			return;

		VulkanSwapChain& swapChain = GetRenderContext()->As<VulkanContext>()->GetSwapChain();
		m_LastSwapChainResult = swapChain.Present(m_SignalSemaphores[m_CurrentFrameIndex], m_ImageIndex);
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;
	}
	
	RenderContextResultCodes VulkanRenderer::WaitAndPresent() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::WaitAndPresent");
		if (GetRendererConfig().ThreadingPolicy == ThreadingPolicy::Multithreaded) {
			const Ref<RenderThread>& renderThread = GetRenderThread();
			//renderThread->SubmitCommandQueue(m_CommandQueue);
			renderThread->SignalToPresent();

			const auto WaitUntilFrameIsFinished = []() {
			};

			WaitUntilFrameIsFinished();
			return renderThread->GetResultCode();
		}

		BeginFrame();
		FlushCommandQueue();
		FlushSubmitQueue();
		RenderFrame();
		EndFrame();
		FlushDeletionQueue();

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
		for (uint32_t i = 0; i < m_MaxFramesInFlight; i++) {
			m_WaitSemaphores[i].Destroy();
			m_SignalSemaphores[i].Destroy();
			m_InFlightFences[i].Destroy();
		}

		auto& allocator = GetRenderDevice()->As<VulkanRenderDevice>()->GetAllocator();
		allocator.DestroyBuffer(s_IDBuffer, s_IDBufferVma);

		RendererBackend::Destroy();
	}

	// Should not be used in a loop 
	void VulkanRenderer::DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
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

		VulkanSwapChain& swapChain = GetRenderContext()->As<VulkanContext>()->GetSwapChain();
		swapChain.Recreate();

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
		m_ImGuiPass.Init(GetRenderContext());
	}

	void VulkanRenderer::RenderImGui() {
		EnqueueToRenderThread([this](RenderCommandList& cmdList) {
			m_ImGuiPass.Render(GetRenderContext()->As<VulkanContext>()->GetSwapChain(), cmdList);
		});
	}
}