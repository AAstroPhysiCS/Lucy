#include "lypch.h"
#include "VulkanRenderer.h"

#include "Context/VulkanSwapChain.h"
#include "Context/VulkanDevice.h"

#include "Device/VulkanRenderDevice.h"
#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Memory/VulkanAllocator.h"

#include "Renderer.h"

#include "Scene/Entity.h"

namespace Lucy {
	
	VulkanRenderer::VulkanRenderer(RenderArchitecture arch, Ref<Window>& window)
		: RendererBase(arch, window) {
	}

	void VulkanRenderer::BeginScene(Ref<Scene>& scene) {
		m_RenderDevice->DispatchCommands();

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.BeginFrame();
	}

	void VulkanRenderer::RenderScene() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkResult lastSwapChainResult = swapChain.GetLastSwapChainResult();
		if (lastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || lastSwapChainResult == VK_SUBOPTIMAL_KHR)
			return;
		m_RenderDevice->ExecuteCommandQueue();
	}

	PresentResult VulkanRenderer::EndScene() {
		//TODO: profiling with optick
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.EndFrame(m_RenderDevice->m_RenderDeviceCommandList->GetCommandQueue());
		return (PresentResult)swapChain.Present();
	}

	void VulkanRenderer::WaitForDevice() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkDeviceWaitIdle(device));
	}

	// Should not be used in a loop 
	void VulkanRenderer::DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		m_RenderDevice.As<VulkanRenderDevice>()->ExecuteSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);
		});
	}

	// Should not be used in a loop 
	void VulkanRenderer::ExecuteSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func) {
		m_RenderDevice.As<VulkanRenderDevice>()->ExecuteSingleTimeCommand(std::move(func));
	}

	void VulkanRenderer::OnWindowResize() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.Recreate();

		m_RenderDevice->Recreate();
	}

	void VulkanRenderer::OnViewportResize() {
		WaitForDevice();
	}
	
	//TODO: Optimization, maybe not recreate the buffers?
	Entity VulkanRenderer::OnMousePicking(Ref<Scene>& scene, const Ref<Pipeline>& idPipeline) {
		VkBuffer temporaryBufferHandle = VK_NULL_HANDLE;
		VmaAllocation temporaryBufferHandleVma = VK_NULL_HANDLE;

		auto& image = idPipeline->GetFrameBuffer().As<VulkanFrameBuffer>()->GetImages()[VulkanSwapChain::Get().GetCurrentFrameIndex()];
		uint32_t imageWidth = image->GetWidth();
		uint32_t imageHeight = image->GetHeight();
		uint64_t imageSize = (uint64_t)imageWidth * imageHeight * 4;
		
		VulkanAllocator::Get().CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, temporaryBufferHandle, temporaryBufferHandleVma);

		image->TransitionImageLayout(image->GetVulkanHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		ExecuteSingleTimeCommand([=](VkCommandBuffer commandBuffer) mutable {
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent.width = imageWidth;
			region.imageExtent.height = imageHeight;
			region.imageExtent.depth = 1;

			vkCmdCopyImageToBuffer(commandBuffer, image->GetVulkanHandle(), image->GetCurrentLayout(), temporaryBufferHandle, 1, &region);
		});

		VmaAllocator vmaInstance = VulkanAllocator::Get().GetVmaInstance();
		uint8_t* rawData = new uint8_t[imageSize];

		void* rawDataMapped;
		vmaMapMemory(vmaInstance, temporaryBufferHandleVma, &rawDataMapped);
		memcpy(rawData, rawDataMapped, imageSize);
		vmaUnmapMemory(vmaInstance, temporaryBufferHandleVma);

		auto& [viewportMouseX, viewportMouseY] = Renderer::GetViewportMousePos();
		uint32_t bufferPos = 4 * ((viewportMouseY * imageWidth) + viewportMouseX);
		glm::vec3 meshID = glm::vec3(rawData[bufferPos], rawData[bufferPos + 1], rawData[bufferPos + 2]);

		image->TransitionImageLayout(image->GetVulkanHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vmaDestroyBuffer(VulkanAllocator::Get().GetVmaInstance(), temporaryBufferHandle, temporaryBufferHandleVma);
		delete[] rawData;

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