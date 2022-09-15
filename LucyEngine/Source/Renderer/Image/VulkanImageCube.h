#pragma once

#include "VulkanImage.h"

#include "Renderer/RenderPass.h"
#include "Renderer/Memory/Buffer/FrameBuffer.h"
#include "Renderer/Context/GraphicsPipeline.h"
#include "Renderer/Mesh.h"

namespace Lucy {

	class VulkanImage2D;

	class VulkanImageCube : public VulkanImage {
	public:
		VulkanImageCube(const std::string& path, ImageCreateInfo& createInfo);
		VulkanImageCube(ImageCreateInfo& createInfo);
		virtual ~VulkanImageCube() = default;

		void Destroy() final override;
		void Recreate(uint32_t width, uint32_t height);
	private:
		void RenderSingleTime();

		void CreateFromPath();
		void CreateEmptyImage();

		Ref<VulkanImage2D> m_HDRImage = nullptr;

		Ref<RenderPass> m_RenderPass = nullptr;
		Ref<FrameBuffer> m_FrameBuffer = nullptr;
		Ref<GraphicsPipeline> m_Pipeline = nullptr;
		Ref<Mesh> m_CubeMesh = nullptr;
	};
}