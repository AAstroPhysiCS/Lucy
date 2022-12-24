#include "lypch.h"
#include "VulkanRenderer.h"

#include "Context/GraphicsPipeline.h"
#include "Context/VulkanSwapChain.h"
#include "Context/VulkanContextDevice.h"

#include "Commands/VulkanCommandQueue.h"
#include "Device/VulkanRenderDevice.h"

#include "Memory/Buffer/Buffer.h"
#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Renderer.h"

#include "Scene/Entity.h"

namespace Lucy {
	
	VulkanRenderer::VulkanRenderer(RenderArchitecture arch, Ref<Window>& window)
		: RendererBase(arch, window) {
		m_WaitSemaphores.resize(m_MaxFramesInFlight);
		m_SignalSemaphores.resize(m_MaxFramesInFlight);
		m_InFlightFences.resize(m_MaxFramesInFlight);
	}

	void VulkanRenderer::BeginScene(Ref<Scene>& scene) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::BeginScene");

		m_RenderDevice->DispatchCommands();

		const auto& device = VulkanContextDevice::Get();
		VkDevice deviceVulkanHandle = device.GetLogicalDevice();

		vkWaitForFences(deviceVulkanHandle, 1, &m_InFlightFences[m_CurrentFrameIndex].GetFence(), VK_TRUE, UINT64_MAX);
		vkResetFences(deviceVulkanHandle, 1, &m_InFlightFences[m_CurrentFrameIndex].GetFence());

		m_LastSwapChainResult = VulkanSwapChain::Get().AcquireNextImage(m_WaitSemaphores[m_CurrentFrameIndex].GetSemaphore(), m_ImageIndex);
		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR)
			return;

		m_RenderTime = m_RenderDevice->GetTimestampResults();
	}

	void VulkanRenderer::RenderScene() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::RenderScene");

		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR)
			return;
		
		m_RenderDevice->ExecuteCommandQueue();
	}

	RenderContextResultCodes VulkanRenderer::EndScene() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::EndScene");

		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR)
			return (RenderContextResultCodes)m_LastSwapChainResult;

		Fence currentFrameFence = m_InFlightFences[m_CurrentFrameIndex];
		Semaphore currentFrameWaitSemaphore = m_WaitSemaphores[m_CurrentFrameIndex];
		Semaphore currentFrameSignalSemaphore = m_SignalSemaphores[m_CurrentFrameIndex];

		VulkanContextDevice& contextDevice = VulkanContextDevice::Get();
		m_RenderDevice->SubmitWorkToGPU(contextDevice.GetGraphicsQueue(), currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		RenderContextResultCodes result = (RenderContextResultCodes) swapChain.Present(m_SignalSemaphores[m_CurrentFrameIndex], m_ImageIndex);
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;

		return result;
	}

	void VulkanRenderer::ExecuteBarrier(void* commandBufferHandle, Ref<Image> image) {
		const auto& vulkanImage = image.As<VulkanImage>();
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

	void VulkanRenderer::WaitForDevice() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::WaitForDevice");
		LUCY_VK_ASSERT(vkDeviceWaitIdle(VulkanContextDevice::Get().GetLogicalDevice()));
	}

	void VulkanRenderer::Destroy() {
		for (uint32_t i = 0; i < m_MaxFramesInFlight; i++) {
			m_WaitSemaphores[i].Destroy();
			m_SignalSemaphores[i].Destroy();
			m_InFlightFences[i].Destroy();
		}

		VulkanAllocator::Get().DestroyBuffer(m_IDBuffer, m_IDBufferVma);

		RendererBase::Destroy();
	}

	// Should not be used in a loop 
	void VulkanRenderer::DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		m_RenderDevice.As<VulkanRenderDevice>()->SubmitImmediateCommand([&](VkCommandBuffer commandBuffer) {
			VkBufferCopy copyRegion = VulkanAPI::BufferCopy(0, 0, size);
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);
		});
	}

	// Should not be used in a loop 
	void VulkanRenderer::SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func) {
		m_RenderDevice.As<VulkanRenderDevice>()->SubmitImmediateCommand(std::move(func));
	}

	void VulkanRenderer::OnWindowResize() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::OnWindowResize");

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.Recreate();

		m_RenderDevice->Recreate();

		auto& allocator = VulkanAllocator::Get();
		allocator.DestroyBuffer(m_IDBuffer, m_IDBufferVma);

		m_IDBuffer = VK_NULL_HANDLE;
		m_IDBufferVma = VK_NULL_HANDLE;
	}

	void VulkanRenderer::OnViewportResize() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::OnViewportResize");

		WaitForDevice();

		auto& allocator = VulkanAllocator::Get();
		allocator.DestroyBuffer(m_IDBuffer, m_IDBufferVma);

		m_IDBuffer = VK_NULL_HANDLE;
		m_IDBufferVma = VK_NULL_HANDLE;
	}
	
	Entity VulkanRenderer::OnMousePicking(Ref<Scene>& scene, const Ref<GraphicsPipeline>& idPipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderer::OnMousePicking");

		auto& image = idPipeline->GetFrameBuffer().As<VulkanFrameBuffer>()->GetImages()[m_CurrentFrameIndex];
		uint32_t imageWidth = image->GetWidth();
		uint32_t imageHeight = image->GetHeight();
		VkDeviceSize imageSize = (VkDeviceSize)imageWidth * imageHeight * 4;

		if (!m_IDBuffer)
			VulkanAllocator::Get().CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, m_IDBuffer, m_IDBufferVma);

		image->SetLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		image->CopyImageToBuffer(m_IDBuffer);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		Buffer<uint8_t> rawData;

		void* rawDataMapped;
		allocator.MapMemory(m_IDBufferVma, rawDataMapped);
		rawData.SetData((uint8_t*)rawDataMapped, imageSize);
		allocator.UnmapMemory(m_IDBufferVma);

		auto [viewportMouseX, viewportMouseY] = Renderer::GetViewportMousePos();
		uint32_t bufferPos = (uint32_t) (4 * ((viewportMouseY * imageWidth) + viewportMouseX));
		glm::vec3 meshID = glm::vec3(rawData[bufferPos], rawData[bufferPos + 1], rawData[bufferPos + 2]);

		image->SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//checking if the data that is being read make sense
		if ((meshID.x > 255.0f || meshID.x < 0.0f) ||
			(meshID.y > 255.0f || meshID.y < 0.0f) ||
			(meshID.z > 255.0f || meshID.z < 0.0f))
			return {};

		//checking if we clicked on the void
		if (meshID.x == 0 && meshID.y == 0 && meshID.z == 0)
			return {};

		Entity selectedEntity = scene->GetEntityByMeshID(meshID);
		return selectedEntity;
	}
}