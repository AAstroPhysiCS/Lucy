#pragma once

#include "VulkanImage.h"

#include "Renderer/RenderPass.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Renderer/Context/GraphicsPipeline.h"

#include "Renderer/Mesh.h"

namespace Lucy {

	class ComputePipeline;

	class VulkanImage2D;

	class VulkanImageCube : public VulkanImage {
	public:
		VulkanImageCube(const std::string& path, ImageCreateInfo& createInfo);
		VulkanImageCube(ImageCreateInfo& createInfo);
		virtual ~VulkanImageCube() = default;

		void Destroy() final override;
		void Recreate(uint32_t width, uint32_t height);

		inline const VulkanImageView& GetIrradianceView() const { return m_IrradianceImageView; }

		Ref<Mesh> GetCubeMesh() const { return m_CubeMesh; }
	private:
		void PrepareForRendering();

		void CreateFromPath();
		void CreateEmptyImage();
		
		Ref<VulkanImage2D> m_HDRImage = nullptr;

		VkImage m_IrradianceImageVulkanHandle = VK_NULL_HANDLE;
		VmaAllocation m_IrradianceImageVma = VK_NULL_HANDLE;
		VulkanImageView m_IrradianceImageView;

		VkImage m_PrefilterImageVulkanHandle = VK_NULL_HANDLE;
		VmaAllocation m_PrefilterImageVma = VK_NULL_HANDLE;
		VulkanImageView m_PrefilterImageView;

#if USE_COMPUTE_FOR_CUBEMAP_GEN
		Ref<ComputePipeline> m_IrradianceComputePipeline = nullptr;
		Ref<ComputePipeline> m_PrefilterComputePipeline = nullptr;
#else
		Ref<GraphicsPipeline> m_IrradiancePipeline = nullptr;
		Ref<GraphicsPipeline> m_PrefilterPipeline = nullptr;
#endif

		Ref<RenderPass> m_RenderPass = nullptr;
		Ref<FrameBuffer> m_FrameBuffer = nullptr;
		Ref<GraphicsPipeline> m_Pipeline = nullptr;
		Ref<Mesh> m_CubeMesh = nullptr;
	};
}