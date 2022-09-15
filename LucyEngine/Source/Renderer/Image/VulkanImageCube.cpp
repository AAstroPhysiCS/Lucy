#include "lypch.h"
#include "VulkanImageCube.h"

#include "VulkanImage2D.h"
#include "Renderer/Renderer.h"
#include "Renderer/VulkanRenderPass.h"
#include "Renderer/Memory/VulkanAllocator.h"
#include "Renderer/Shader/ShaderLibrary.h"

#include "stb/stb_image.h"

#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

namespace Lucy {

	extern void RenderEnvironmentalCube(void* commandBuffer, Ref<GraphicsPipeline> pipeline, RenderCommand* command);

	VulkanImageCube::VulkanImageCube(const std::string& path, ImageCreateInfo& createInfo)
		: VulkanImage(path, createInfo) {
		if (m_CreateInfo.ImageType != ImageType::TypeCubeColor)
			LUCY_ASSERT(false);

		if (!stbi_is_hdr(m_Path.c_str())) {
			LUCY_CRITICAL(fmt::format("The texture isnt HDR. Texture path: {0}", m_Path));
			LUCY_ASSERT(false);
		}

		ImageCreateInfo hdrImageCreateInfo;
		hdrImageCreateInfo.Format = m_CreateInfo.Format;
		hdrImageCreateInfo.ImageType = ImageType::Type2DColor;
		hdrImageCreateInfo.Parameter = m_CreateInfo.Parameter;
		hdrImageCreateInfo.GenerateSampler = true;
		m_HDRImage = Image::Create(path, hdrImageCreateInfo).As<VulkanImage2D>();

		//the resolution of the hdr image. its an arbitrary number
		m_Width = 2048;
		m_Height = 2048;

		Renderer::EnqueueToRenderThread([=]() {
			CreateFromPath();
			RenderSingleTime();
		});
	}

	VulkanImageCube::VulkanImageCube(ImageCreateInfo& createInfo)
		: VulkanImage(createInfo) {
		if (m_CreateInfo.ImageType != ImageType::TypeCubeColor)
			LUCY_ASSERT(false);
		Renderer::EnqueueToRenderThread([=]() {
			CreateEmptyImage();
		});
	}

	void VulkanImageCube::CreateFromPath() {
		uint32_t faceCount = 6;

		VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | m_CreateInfo.Flags;

		if (m_CreateInfo.GenerateSampler)
			flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		if (m_CreateInfo.GenerateMipmap)
			flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, faceCount);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmaps();
		else //transitioning only then, when we dont care about mipmapping. Mipmapping already transitions to the right layout#
			SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		CreateVulkanImageViewHandle();
	}

	void VulkanImageCube::CreateEmptyImage() {
		CreateFromPath();
	}

	void VulkanImageCube::RenderSingleTime() {
		static ImageCreateInfo frameBufferImageCreateInfo {
			.Width = m_Width,
			.Height = m_Height,
			.ImageType = ImageType::Type2DArrayColor,
			.Format = ImageFormat::R8G8B8A8_UNORM,
			.Parameter {
				.U = ImageAddressMode::REPEAT,
				.V = ImageAddressMode::REPEAT,
				.W = ImageAddressMode::REPEAT,
				.Min = ImageFilterMode::LINEAR,
				.Mag = ImageFilterMode::LINEAR,
			},
			.Layers = 6,
			.GenerateSampler = true
		};
		static RenderPassLayout renderPassLayout;
		static auto colorAttachment = RenderPassLayout::Attachment {
			.Format = frameBufferImageCreateInfo.Format,
			.Samples = 1,
			.LoadOp = RenderPassLoadOp::Clear,
			.StoreOp = RenderPassStoreOp::Store,
			.StencilLoadOp = RenderPassLoadOp::DontCare,
			.StencilStoreOp = RenderPassStoreOp::DontCare,
			.Initial = VK_IMAGE_LAYOUT_UNDEFINED,
			.Final = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR,
			.Reference = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
		};
		renderPassLayout.ColorAttachments = std::vector(1, colorAttachment);

		static RenderPassCreateInfo renderPassCreateInfo {
			.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
			.Layout = renderPassLayout,
			.Multiview {
				.ViewMask = 0b00111111,
				.CorrelationMask = 0b00000000
			}
		};
		
		m_RenderPass = RenderPass::Create(renderPassCreateInfo);

		static FrameBufferCreateInfo frameBufferCreateInfo {
			.Width = m_Width,
			.Height = m_Height,
			.RenderPass = m_RenderPass,
			.ImageBuffers {
				Image::Create(frameBufferImageCreateInfo),
			}
		};

		m_FrameBuffer = FrameBuffer::Create(frameBufferCreateInfo);

		static GraphicsPipelineCreateInfo pipelineCreateInfo {
			.Topology = Topology::TRIANGLES,
			.Rasterization = { true, CullingMode::None, 1.0f, PolygonMode::FILL },
			.VertexShaderLayout = {
				{ "a_Pos", ShaderDataSize::Float3 }
			},
			.RenderPass = m_RenderPass,
			.FrameBuffer = m_FrameBuffer,
			.Shader = ShaderLibrary::Get().GetShader("LucyHDR"),
		};

		m_Pipeline = GraphicsPipeline::Create(pipelineCreateInfo);

		static std::vector<float> vertices = {
			//back face
			-1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, 1.0f, -1.0f,
			// front face
			-1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f,
			// left face
			-1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,
			// right face
			1.0f, 1.0f, 1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,
			// bottom face
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, -1.0f,
			// top face
			-1.0f, 1.0f, -1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, -1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, -1.0f,
			-1.0f, 1.0f, 1.0f
		};

		static std::vector<uint32_t> indices(vertices.size());
		for (uint32_t i = 0; i < indices.size(); i++)
			indices[i] = i;

		m_CubeMesh = Mesh::Create(vertices, indices);

		Renderer::EnqueueToRenderThread([=]() {
			const VulkanImageView& hdrImageView = m_HDRImage->GetImageView();
			CommandResourceHandle commandResourceHandle = Renderer::CreateCommandResource(RenderEnvironmentalCube, m_Pipeline);
			Renderer::EnqueueCommand<InternalRenderCommand>(commandResourceHandle, hdrImageView.GetVulkanHandle(), m_HDRImage->GetCurrentLayout(), hdrImageView.GetSampler(), m_CubeMesh);
			Renderer::EnqueueResourceFree(commandResourceHandle); //since its singletime
		});
	}

	void VulkanImageCube::Recreate(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		Destroy();

		if (!m_Path.empty())
			CreateFromPath();
		else
			CreateEmptyImage();
	}

	void VulkanImageCube::Destroy() {
		if (!m_Image)
			return;
		m_HDRImage->Destroy();
		m_ImageView.Destroy();

		VulkanAllocator::Get().DestroyImage(m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;

		m_CubeMesh->Destroy();
		m_RenderPass->Destroy();
		m_FrameBuffer->Destroy();
		m_Pipeline->Destroy();
	}
}