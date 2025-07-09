#include "lypch.h"
#include "Components.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderGraph/RenderGraphResource.h"
#include "Renderer/RendererPasses.h"

namespace Lucy {

	void TransformComponent::CalculateMatrix() {
		m_Mat = glm::translate(glm::mat4(1.0f), m_Position)
			* glm::toMat4(glm::quat(glm::radians(m_Rotation)))
			* glm::scale(glm::mat4(1.0f), m_Scale);
	}

	void MeshComponent::LoadMesh(const std::string& path) {
		m_Mesh = std::move(Mesh::Create(path));
	}

	void HDRCubemapComponent::LoadCubemap(const std::filesystem::path& path) {
		Renderer::EnqueueToRenderCommandQueue([&, path](const Ref<RenderDevice>& device) {
			ImageCreateInfo hdrCreateInfo = {
				.Width = CubemapPass::HDRImageWidth,
				.Height = CubemapPass::HDRImageHeight,
				.ImageType = ImageType::TypeCube,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Format = ImageFormat::R32G32B32A32_SFLOAT,
				.Parameter = {
					.U = ImageAddressMode::REPEAT,
					.V = ImageAddressMode::REPEAT,
					.W = ImageAddressMode::REPEAT,
					.Min = ImageFilterMode::LINEAR,
					.Mag = ImageFilterMode::LINEAR,
				},
				.GenerateSampler = true,
				.GenerateMipmap = false //does not support it yet
			};

			ImageCreateInfo irradianceImageCreateInfo = {
				.Width = CubemapPass::HDRImageWidth,
				.Height = CubemapPass::HDRImageHeight,
				.ImageType = ImageType::TypeCube,
#if USE_COMPUTE_FOR_CUBEMAP_GEN
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
#else
				.ImageUsage = ImageUsage::AsColorAttachment,
#endif
				.Format = ImageFormat::R16G16B16A16_SFLOAT,
				.GenerateSampler = true,
				.GenerateMipmap = false,
				.ImGuiUsage = false
			};

			ImageCreateInfo originalImageCreateInfo{
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorTransferAttachment,
				.Format = hdrCreateInfo.Format,
				.Parameter = hdrCreateInfo.Parameter,
				.GenerateSampler = true
			};

			auto originalImageHandle = device->CreateImage(path, originalImageCreateInfo);
			m_CubemapImageHandle = device->CreateImage(path, hdrCreateInfo);

			Renderer::ImportExternalRenderGraphTransientResource(RGResource(OriginalHDRImage), originalImageHandle);
			Renderer::ImportExternalRenderGraphResource(RGResource(HDRCubeImage), m_CubemapImageHandle);
#if USE_COMPUTE_FOR_CUBEMAP_GEN
			m_IrradianceImageHandle = device->CreateImage(irradianceImageCreateInfo);
			Renderer::ImportExternalRenderGraphResource(RGResource(IrradianceImage), m_IrradianceImageHandle);
#endif
		});
	}
}