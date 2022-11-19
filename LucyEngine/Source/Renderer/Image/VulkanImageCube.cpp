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
	extern void ComputeIrradiance(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command);
	extern void ComputePrefilter(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command);

	VulkanImageCube::VulkanImageCube(const std::string& path, ImageCreateInfo& createInfo)
		: VulkanImage(path, createInfo) {
		m_LayerCount = 6;

		if (m_CreateInfo.ImageType != ImageType::TypeCubeColor)
			LUCY_ASSERT(false);

		if (!stbi_is_hdr(m_Path.c_str())) {
			LUCY_CRITICAL(fmt::format("The texture isnt HDR. Texture path: {0}", m_Path));
			LUCY_ASSERT(false);
		}

		ImageCreateInfo hdrImageCreateInfo{
			.ImageType = ImageType::Type2DColor,
			.Format = m_CreateInfo.Format,
			.Parameter = m_CreateInfo.Parameter,
			.GenerateSampler = true
		};
		m_HDRImage = Image::Create(path, hdrImageCreateInfo).As<VulkanImage2D>();

		//the resolution of the hdr image. its an arbitrary number
		m_Width = 1024;
		m_Height = 1024;

		ImageCreateInfo frameBufferImageCreateInfo {
			.Width = m_Width,
			.Height = m_Height,
			.ImageType = ImageType::Type2DArrayColor,
			.Format = m_CreateInfo.Format,
			.Parameter = m_CreateInfo.Parameter,
			.Flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			.Layers = 6,
			.GenerateSampler = true
		};

		RenderPassLayout renderPassLayout {
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

		RenderPassCreateInfo renderPassCreateInfo {
			.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
			.Layout = renderPassLayout,
			.Multiview {
				.ViewMask = 0b00111111,
				.CorrelationMask = 0b00000011
			}
		};

		m_RenderPass = RenderPass::Create(renderPassCreateInfo);

		FrameBufferCreateInfo frameBufferCreateInfo {
			.Width = m_Width,
			.Height = m_Height,
			.RenderPass = m_RenderPass,
			.ImageBuffers {
				Image::Create(frameBufferImageCreateInfo),
			}
		};

		m_FrameBuffer = FrameBuffer::Create(frameBufferCreateInfo);

		GraphicsPipelineCreateInfo pipelineCreateInfo {
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
		m_LayerCount = 6;
		if (m_CreateInfo.ImageType != ImageType::TypeCubeColor)
			LUCY_ASSERT(false);
		Renderer::EnqueueToRenderThread([=]() {
			CreateEmptyImage();
		});
	}

	void VulkanImageCube::CreateFromPath() {
		//Storage bit is for compute shaders (for irradiance and co.)
		VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | m_CreateInfo.Flags;

		if (m_CreateInfo.GenerateSampler)
			flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_LayerCount);
		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmaps();
		else //transitioning only then, when we dont care about mipmapping. Mipmapping already transitions to the right layout#
			SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 1, m_LayerCount);

		CreateVulkanImageViewHandle();
	}

	void VulkanImageCube::CreateEmptyImage() {
		CreateFromPath();
	}

	void VulkanImageCube::PrepareForRendering() {
		const VulkanImageView& hdrImageView = m_HDRImage->GetImageView();

		CommandResourceHandle commandResourceHandle = Renderer::CreateCommandResource(m_Pipeline, PrepareEnvironmentalCube);

		auto imageBuffer = m_Pipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_EquirectangularMap");
		imageBuffer->BindImage(hdrImageView.GetVulkanHandle(), m_HDRImage->GetCurrentLayout(), hdrImageView.GetSampler());

		Renderer::UpdateDescriptorSets(m_Pipeline);

		Renderer::EnqueueCommand<CubeRenderCommand>(commandResourceHandle, hdrImageView.GetVulkanHandle(), m_HDRImage->GetCurrentLayout(), hdrImageView.GetSampler(), m_CubeMesh);
		Renderer::EnqueueCommandResourceFree(commandResourceHandle); //since its singletime

		//copying the layered color attachment, to a sampler2DCube
		Renderer::EnqueueToRenderThread([=]() {
			Ref<VulkanFrameBuffer> preparedFrameBuffer = m_Pipeline->GetFrameBuffer();
			Ref<VulkanImage2D> preparedImage = preparedFrameBuffer->GetImages()[0]; //we are sure, that we have only 1 image

			preparedImage->SetLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 0, 1, m_LayerCount);
			SetLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0, 1, m_LayerCount);

			std::vector<VkImageCopy> regions;
			regions.reserve(m_LayerCount);

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

			SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 1, m_LayerCount);

			m_Pipeline->Destroy();
			m_RenderPass->Destroy();
			m_FrameBuffer->Destroy();

			m_HDRImage->Destroy();
		});
#pragma region Irradiance

		Renderer::EnqueueToRenderThread([&]() {
			ImageFormat oldFormat = m_CreateInfo.Format;
			m_CreateInfo.Format = ImageFormat::R16G16B16A16_SFLOAT;

			VulkanAllocator& allocator = VulkanAllocator::Get();
			allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), VK_IMAGE_LAYOUT_UNDEFINED,
										   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TYPE_2D, m_IrradianceImageVulkanHandle, m_IrradianceImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_LayerCount);
			CreateVulkanImageViewHandle(m_IrradianceImageView, m_IrradianceImageVulkanHandle);
			m_CreateInfo.Format = oldFormat;
		});

#if USE_COMPUTE_FOR_CUBEMAP_GEN
		m_IrradianceComputePipeline = ComputePipeline::Create({ ShaderLibrary::Get().GetShader("LucyIrradianceGen").As<VulkanComputeShader>() });

		Renderer::EnqueueToRenderThread([&]() {
			SetLayout(VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, m_LayerCount);
			TransitionImageLayout(m_IrradianceImageVulkanHandle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, m_LayerCount);

			const auto& environmentMap = m_IrradianceComputePipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_EnvironmentMap");
			environmentMap->BindImage(m_ImageView.GetVulkanHandle(), m_CurrentLayout, m_ImageView.GetSampler());

			const auto& environmentIrradianceMap = m_IrradianceComputePipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_EnvironmentIrradianceMap");
			environmentIrradianceMap->BindImage(m_IrradianceImageView.GetVulkanHandle(), m_CurrentLayout, m_IrradianceImageView.GetSampler());

			Renderer::UpdateDescriptorSets(m_IrradianceComputePipeline);

			CommandResourceHandle computeHandle = Renderer::CreateCommandResource(m_IrradianceComputePipeline, ComputeIrradiance);

			constexpr uint32_t workGroupSize = 32;
			Renderer::EnqueueCommand<ComputeDispatchCommand>(computeHandle, Priority::HIGH, m_Width / workGroupSize, m_Height / workGroupSize, m_LayerCount);
			Renderer::EnqueueCommandResourceFree(computeHandle);

			//TODO: Investigate the slow performance
			//TODO: Investigate, Should I set the layout to VK_IMAGE_SHADER_READ_ONLY_OPTIMAL after irradiance pass?
		});
#else
		ImageCreateInfo irradianceTextureCreateInfo = {
			.Width = m_Width,
			.Height = m_Height,
			.ImageType = ImageType::Type2DArrayColor,
			.Format = ImageFormat::R16G16B16A16_SFLOAT,
			.Parameter {
				.U = ImageAddressMode::REPEAT,
				.V = ImageAddressMode::REPEAT,
				.W = ImageAddressMode::REPEAT,
				.Min = ImageFilterMode::LINEAR,
				.Mag = ImageFilterMode::LINEAR,
			},
			.Layers = 6,
			.ImGuiUsage = false,
			.GenerateSampler = true
		};

		RenderPassLayout irradiancePassLayout{
			.ColorAttachments = {
				RenderPassLayout::Attachment {
					.Format = irradianceTextureCreateInfo.Format,
					.Samples = 1,
					.LoadOp = RenderPassLoadOp::Clear,
					.StoreOp = RenderPassStoreOp::Store,
					.StencilLoadOp = RenderPassLoadOp::DontCare,
					.StencilStoreOp = RenderPassStoreOp::DontCare,
					.Initial = VK_IMAGE_LAYOUT_UNDEFINED,
					.Final = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR,
					.Reference = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
				},
			},
		};

		RenderPassCreateInfo irradianceRenderPassCreateInfo{
			.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
			.Layout = irradiancePassLayout,
			.Multiview {
				.ViewMask = 0b00111111,
				.CorrelationMask = 0b00000011
			}
		};
		Ref<RenderPass> irradianceRenderPass = RenderPass::Create(irradianceRenderPassCreateInfo);

		FrameBufferCreateInfo irradianceFrameBufferCreateInfo{
			.Width = m_Width,
			.Height = m_Height,
			.IsInFlight = false,
			.RenderPass = irradianceRenderPass,
			.ImageBuffers {
				Image::Create(irradianceTextureCreateInfo)
			},
		};
		Ref<VulkanFrameBuffer> irradianceFrameBuffer = FrameBuffer::Create(irradianceFrameBufferCreateInfo).As<VulkanFrameBuffer>();

		GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{
			.Topology = Topology::TRIANGLES,
			.Rasterization = {
				.DisableBackCulling = true,
				.CullingMode = CullingMode::None,
				.LineWidth = 1.0f,
				.PolygonMode = PolygonMode::FILL
			},
			.VertexShaderLayout {
				{ "a_Pos", ShaderDataSize::Float3 },
			},
			.RenderPass = irradianceRenderPass,
			.FrameBuffer = irradianceFrameBuffer,
			.Shader = ShaderLibrary::Get().GetShader("LucyIrradianceGen")
		};

		m_IrradiancePipeline = GraphicsPipeline::Create(graphicsPipelineCreateInfo);
		
		Renderer::EnqueueToRenderThread([=]() {
			SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 1, m_LayerCount);
			TransitionImageLayout(irradianceFrameBuffer->GetImages()[0]->GetVulkanHandle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 1, m_LayerCount);

			const auto& environmentMap = m_IrradiancePipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_EnvironmentMap");
			environmentMap->BindImage(m_ImageView.GetVulkanHandle(), m_CurrentLayout, m_ImageView.GetSampler());

			Renderer::UpdateDescriptorSets(m_IrradiancePipeline);
			CommandResourceHandle computeHandle = Renderer::CreateCommandResource(m_IrradiancePipeline, ComputeIrradiance);

			Renderer::EnqueueCommand<CubeRenderCommand>(computeHandle, m_IrradianceImageView.GetVulkanHandle(), VK_IMAGE_LAYOUT_GENERAL, m_IrradianceImageView.GetSampler(), m_CubeMesh);
			Renderer::EnqueueCommandResourceFree(computeHandle);
		});
#endif
#pragma endregion Irradiance

#pragma region BRDF

#pragma endregion BRDF

#pragma region Prefiltering

#if USE_COMPUTE_FOR_CUBEMAP_GEN //should not be used yet
		Renderer::EnqueueToRenderThread([&]() {
			//regardless of what the user wants, we need to generate mipmaps for prefiltering
			m_CreateInfo.GenerateMipmap = true;
			m_MaxMipLevel = 5;

			ImageFormat oldFormat = m_CreateInfo.Format;
			m_CreateInfo.Format = ImageFormat::R16G16B16A16_SFLOAT;

			VkImageLayout tempLayout = m_CurrentLayout;

			VulkanAllocator& allocator = VulkanAllocator::Get();
			allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), VK_IMAGE_LAYOUT_UNDEFINED,
										   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
										   VK_IMAGE_TYPE_2D, m_PrefilterImageVulkanHandle, m_PrefilterImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_LayerCount);

			m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			GenerateMipmaps(m_PrefilterImageVulkanHandle);

			CreateVulkanImageViewHandle(m_PrefilterImageView, m_PrefilterImageVulkanHandle);

			TransitionImageLayout(m_PrefilterImageVulkanHandle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 0, 0, m_MaxMipLevel, m_LayerCount);

			//doing this, since generating mipmaps actually change this objects current layout
			m_CurrentLayout = tempLayout;
			m_CreateInfo.Format = oldFormat;
		});

		m_PrefilterComputePipeline = ComputePipeline::Create({ ShaderLibrary::Get().GetShader("LucyPrefilterGen").As<VulkanComputeShader>() });

		Renderer::EnqueueToRenderThread([&]() {
			const auto& environmentMap = m_PrefilterComputePipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_EnvironmentMap");
			environmentMap->BindImage(m_ImageView.GetVulkanHandle(), m_CurrentLayout, m_ImageView.GetSampler());
			CommandResourceHandle computeHandle = Renderer::CreateCommandResource(m_PrefilterComputePipeline, ComputePrefilter);

			constexpr uint32_t workGroupSize = 32;
			Renderer::EnqueueCommand<ComputeDispatchCommand>(computeHandle, Priority::HIGH, m_Width / workGroupSize, m_Height / workGroupSize, m_LayerCount);
			Renderer::EnqueueCommandResourceFree(computeHandle);
		});
#else
		//TODO: Cache the immediate commands. Current workflow is probably bad for performance?
		//TODO: Do prefiltering without compute shaders, however do irradiance with compute shaders since that is stable
		ImageCreateInfo prefilterTextureCreateInfo = {
			.Width = m_Width,
			.Height = m_Height,
			.ImageType = ImageType::Type2DArrayColor,
			.Format = ImageFormat::R16G16B16A16_SFLOAT,
			.Parameter {
				.U = ImageAddressMode::REPEAT,
				.V = ImageAddressMode::REPEAT,
				.W = ImageAddressMode::REPEAT,
				.Min = ImageFilterMode::LINEAR,
				.Mag = ImageFilterMode::LINEAR,
			},
			.Layers = 6,
			.ImGuiUsage = false,
			.GenerateSampler = true
		};

		RenderPassLayout prefilterPassLayout{
			.ColorAttachments = {
				RenderPassLayout::Attachment {
					.Format = prefilterTextureCreateInfo.Format,
					.Samples = 1,
					.LoadOp = RenderPassLoadOp::Clear,
					.StoreOp = RenderPassStoreOp::Store,
					.StencilLoadOp = RenderPassLoadOp::DontCare,
					.StencilStoreOp = RenderPassStoreOp::DontCare,
					.Initial = VK_IMAGE_LAYOUT_UNDEFINED,
					.Final = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR,
					.Reference = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
				},
			},
		};

		RenderPassCreateInfo prefilterRenderPassCreateInfo{
			.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
			.Layout = prefilterPassLayout,
			.Multiview {
				.ViewMask = 0b00111111,
				.CorrelationMask = 0b00000011
			}
		};
		Ref<RenderPass> prefilterRenderPass = RenderPass::Create(prefilterRenderPassCreateInfo);

		FrameBufferCreateInfo prefilterFrameBufferCreateInfo{
			.Width = m_Width,
			.Height = m_Height,
			.IsInFlight = false,
			.RenderPass = prefilterRenderPass,
			.ImageBuffers {
				Image::Create(prefilterTextureCreateInfo)
			},
		};
		Ref<VulkanFrameBuffer> prefilterFrameBuffer = FrameBuffer::Create(prefilterFrameBufferCreateInfo).As<VulkanFrameBuffer>();

		GraphicsPipelineCreateInfo prefilterGraphicsPipelineCreateInfo{
			.Topology = Topology::TRIANGLES,
			.Rasterization = {
				.DisableBackCulling = true,
				.CullingMode = CullingMode::None,
				.LineWidth = 1.0f,
				.PolygonMode = PolygonMode::FILL
			},
			.VertexShaderLayout {
				{ "a_Pos", ShaderDataSize::Float3 },
			},
			.RenderPass = prefilterRenderPass,
			.FrameBuffer = prefilterFrameBuffer,
			.Shader = ShaderLibrary::Get().GetShader("LucyPrefilterGen")
		};

		m_PrefilterPipeline = GraphicsPipeline::Create(prefilterGraphicsPipelineCreateInfo);
#endif
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

		m_PrefilterImageView.Destroy();
		allocator.DestroyImage(m_PrefilterImageVulkanHandle, m_PrefilterImageVma);
		m_PrefilterImageVulkanHandle = VK_NULL_HANDLE;

#if USE_COMPUTE_FOR_CUBEMAP_GEN
		if (m_IrradianceComputePipeline)
			m_IrradianceComputePipeline->Destroy();

		if (m_PrefilterComputePipeline)
			m_PrefilterComputePipeline->Destroy();
#endif

		if (m_CubeMesh)
			m_CubeMesh->Destroy();

		m_ImageView.Destroy();
		allocator.DestroyImage(m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;
	}
}