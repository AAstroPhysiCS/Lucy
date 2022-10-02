#include "lypch.h"
#include "VulkanImageCube.h"

#include "VulkanImage2D.h"
#include "Renderer/Renderer.h"
#include "Renderer/VulkanRenderPass.h"

#include "Renderer/Context/GraphicsPipeline.h"
#include "Renderer/Context/ComputePipeline.h"

#include "Renderer/Memory/VulkanAllocator.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanUniformBuffer.h"

#include "Renderer/Shader/ShaderLibrary.h"
#include "Renderer/Shader/VulkanComputeShader.h"

#include "stb/stb_image.h"

#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

namespace Lucy {

	extern void PrepareEnvironmentalCube(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command);
	extern void ComputeIrradiancePass(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command);

	VulkanImageCube::VulkanImageCube(const std::string& path, ImageCreateInfo& createInfo)
		: VulkanImage(path, createInfo) {
		m_LayerCount = 6;

		if (m_CreateInfo.ImageType != ImageType::TypeCubeColor)
			LUCY_ASSERT(false);

		if (!stbi_is_hdr(m_Path.c_str())) {
			LUCY_CRITICAL(fmt::format("The texture isnt HDR. Texture path: {0}", m_Path));
			LUCY_ASSERT(false);
		}

		ImageCreateInfo hdrImageCreateInfo {
			.ImageType = ImageType::Type2DColor,
			.Format = m_CreateInfo.Format,
			.Parameter = m_CreateInfo.Parameter,
			.GenerateSampler = true
		};
		m_HDRImage = Image::Create(path, hdrImageCreateInfo).As<VulkanImage2D>();

		//the resolution of the hdr image. its an arbitrary number
		m_Width = 1024;
		m_Height = 1024;

		ImageCreateInfo frameBufferImageCreateInfo{
			.Width = m_Width,
			.Height = m_Height,
			.ImageType = ImageType::Type2DArrayColor,
			.Format = m_CreateInfo.Format,
			.Parameter = m_CreateInfo.Parameter,
			.Flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			.Layers = 6,
			.GenerateSampler = true
		};

		RenderPassLayout renderPassLayout{
			.ColorAttachments {
				RenderPassLayout::Attachment {
					.Format = frameBufferImageCreateInfo.Format,
					.Samples = 1,
					.LoadOp = RenderPassLoadOp::Clear,
					.StoreOp = RenderPassStoreOp::Store,
					.StencilLoadOp = RenderPassLoadOp::DontCare,
					.StencilStoreOp = RenderPassStoreOp::DontCare,
					.Initial = VK_IMAGE_LAYOUT_UNDEFINED,
					.Final = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR,
					.Reference = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
				}
			}
		};

		RenderPassCreateInfo renderPassCreateInfo{
			.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
			.Layout = renderPassLayout,
			.Multiview {
				.ViewMask = 0b00111111,
				.CorrelationMask = 0b00000011
			}
		};

		m_RenderPass = RenderPass::Create(renderPassCreateInfo);

		FrameBufferCreateInfo frameBufferCreateInfo{
			.Width = m_Width,
			.Height = m_Height,
			.RenderPass = m_RenderPass,
			.ImageBuffers {
				Image::Create(frameBufferImageCreateInfo),
			}
		};

		m_FrameBuffer = FrameBuffer::Create(frameBufferCreateInfo);

		GraphicsPipelineCreateInfo pipelineCreateInfo{
			.Topology = Topology::TRIANGLES,
			.Rasterization = {
				.DisableBackCulling = true,
				.CullingMode = CullingMode::None,
				.LineWidth = 1.0f,
				.PolygonMode = PolygonMode::FILL
			},
			.VertexShaderLayout = {
				{ "a_Pos", ShaderDataSize::Float3 }
			},
			.RenderPass = m_RenderPass,
			.FrameBuffer = m_FrameBuffer,
			.Shader = ShaderLibrary::Get().GetShader("LucyImageToHDRConverter"),
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
			CreateFromPath();
			PrepareForRendering();
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
		m_LayerCount = 6;
		
		//Storage bit is for compute shaders (for irradiance and co.)
		VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | m_CreateInfo.Flags;

		if (m_CreateInfo.GenerateSampler)
			flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		if (m_CreateInfo.GenerateMipmap)
			flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_LayerCount);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmaps();
		else //transitioning only then, when we dont care about mipmapping. Mipmapping already transitions to the right layout#
			SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, m_LayerCount);

		CreateVulkanImageViewHandle();
	}

	void VulkanImageCube::CreateEmptyImage() {
		CreateFromPath();
	}

	void VulkanImageCube::PrepareForRendering() {
		const VulkanImageView& hdrImageView = m_HDRImage->GetImageView();

		CommandResourceHandle commandResourceHandle = Renderer::CreateCommandResource(m_Pipeline, PrepareEnvironmentalCube);
		Renderer::EnqueueCommand<CubeRenderCommand>(commandResourceHandle, hdrImageView.GetVulkanHandle(), m_HDRImage->GetCurrentLayout(), hdrImageView.GetSampler(), m_CubeMesh);
		Renderer::EnqueueCommandResourceFree(commandResourceHandle); //since its singletime

		//copying the layered color attachment, to a sampler2DCube
		Renderer::EnqueueToRenderThread([=]() {
			Ref<VulkanFrameBuffer> preparedFrameBuffer = m_Pipeline->GetFrameBuffer();
			Ref<VulkanImage2D> preparedImage = preparedFrameBuffer->GetImages()[0]; //we are sure, that we have only 1 image

			preparedImage->SetLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1, m_LayerCount);
			SetLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1, m_LayerCount);

			std::vector<VkImageCopy> regions;
			for (uint32_t face = 0; face < m_LayerCount; face++) {
				VkImageCopy region = {
					.srcSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = 0,
						.baseArrayLayer = face,
						.layerCount = 1
					},
					.srcOffset = { 0, 0, 0 },
					.dstSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = 0,
						.baseArrayLayer = face,
						.layerCount = 1
					},
					.dstOffset = { 0, 0, 0 },
					.extent = {
						.width = (uint32_t)m_Width,
						.height = (uint32_t)m_Height,
						.depth = 1
					},
				};
				regions.push_back(region);
			}
			preparedImage->CopyImageToImage(this, regions);

			SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, m_LayerCount);

			m_Pipeline->Destroy();
			m_RenderPass->Destroy();
			m_FrameBuffer->Destroy();

			m_HDRImage->Destroy();
		});

#pragma region Irradiance

		Renderer::EnqueueToRenderThread([=]() {
			VulkanAllocator& allocator = VulkanAllocator::Get();
			allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), VK_IMAGE_LAYOUT_UNDEFINED,
										   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TYPE_2D, m_IrradianceImageVulkanHandle, m_IrradianceImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_LayerCount);
			CreateVulkanImageViewHandle(m_IrradianceImageView, m_IrradianceImageVulkanHandle);
		});

		m_IrradianceComputePipeline = ComputePipeline::Create({ ShaderLibrary::Get().GetShader("LucyIrradianceGen").As<VulkanComputeShader>() });

		Renderer::EnqueueToRenderThread([=]() {
			SetLayout(VK_IMAGE_LAYOUT_GENERAL, 0, 1, m_LayerCount);
			TransitionImageLayout(m_IrradianceImageVulkanHandle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0, 1, m_LayerCount);

			const auto& environmentMap = m_IrradianceComputePipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_EnvironmentMap");
			environmentMap->BindImage(m_ImageView.GetVulkanHandle(), m_CurrentLayout, m_ImageView.GetSampler());

			const auto& environmentIrradianceMap = m_IrradianceComputePipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_EnvironmentIrradianceMap");
			environmentIrradianceMap->BindImage(m_IrradianceImageView.GetVulkanHandle(), m_CurrentLayout, m_IrradianceImageView.GetSampler());
			CommandResourceHandle computeHandle = Renderer::CreateCommandResource(m_IrradianceComputePipeline, ComputeIrradiancePass);

			constexpr uint32_t workGroupSize = 32;
			Renderer::EnqueueCommand<ComputeDispatchCommand>(computeHandle, Priority::HIGH, m_Width / workGroupSize, m_Height / workGroupSize, m_LayerCount);
			Renderer::EnqueueCommandResourceFree(computeHandle);

			//TODO: Investigate the slow performance
			//TODO: Investigate, Should I set the layout to VK_IMAGE_SHADER_READ_ONLY_OPTIMAL after irradiance pass?
		});
#pragma endregion Irradiance

#pragma region BRDF

#pragma endregion BRDF

#pragma region Prefiltering

#pragma endregion Prefiltering
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

		auto& allocator = VulkanAllocator::Get();

		m_IrradianceImageView.Destroy();
		allocator.DestroyImage(m_IrradianceImageVulkanHandle, m_IrradianceImageVma);
		m_IrradianceImageVulkanHandle = VK_NULL_HANDLE;

		if (m_IrradianceComputePipeline)
			m_IrradianceComputePipeline->Destroy();

		if (m_CubeMesh)
			m_CubeMesh->Destroy();
		
		m_ImageView.Destroy();
		allocator.DestroyImage(m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;
	}
}