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

	void MeshComponent::SetMesh(Ref<Mesh> mesh) { 
		m_Mesh = std::move(mesh);
	}

	void HDRCubemapComponent::LoadCubemap(const std::filesystem::path& path) {
		Renderer::EnqueueToRenderThread([&, path](const Ref<RenderDevice>& device) {
			ImageCreateInfo hdrCreateInfo;
			//the resolution of the hdr image. its an arbitrary number
			hdrCreateInfo.Width = 1024;
			hdrCreateInfo.Height = 1024;
			hdrCreateInfo.Format = ImageFormat::R32G32B32A32_SFLOAT;
			hdrCreateInfo.ImageType = ImageType::TypeCubeColor;
			hdrCreateInfo.Parameter.U = ImageAddressMode::REPEAT;
			hdrCreateInfo.Parameter.V = ImageAddressMode::REPEAT;
			hdrCreateInfo.Parameter.W = ImageAddressMode::REPEAT;
			hdrCreateInfo.Parameter.Mag = ImageFilterMode::LINEAR;
			hdrCreateInfo.Parameter.Min = ImageFilterMode::LINEAR;
			hdrCreateInfo.Flags = VK_IMAGE_USAGE_STORAGE_BIT;
			hdrCreateInfo.GenerateSampler = true;
			hdrCreateInfo.GenerateMipmap = false; //does not support it yet

			ImageCreateInfo irradianceImageCreateInfo;
			irradianceImageCreateInfo.Width = CubemapPass::HDRImageWidth;
			irradianceImageCreateInfo.Height = CubemapPass::HDRImageHeight;
			irradianceImageCreateInfo.ImageType = ImageType::TypeCubeColor;
			irradianceImageCreateInfo.Format = ImageFormat::R16G16B16A16_SFLOAT;
			irradianceImageCreateInfo.Flags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			irradianceImageCreateInfo.GenerateSampler = true;
			irradianceImageCreateInfo.GenerateMipmap = false;
			irradianceImageCreateInfo.ImGuiUsage = false;

			ImageCreateInfo originalImageCreateInfo{
				.ImageType = ImageType::Type2DColor,
				.Format = hdrCreateInfo.Format,
				.Parameter = hdrCreateInfo.Parameter,
				.GenerateSampler = true
			};

			auto originalImageHandle = device->CreateImage(path, originalImageCreateInfo);

			m_CubemapImageHandle = device->CreateImage(path, hdrCreateInfo);
			m_IrradianceImageHandle = device->CreateImage(irradianceImageCreateInfo);

			Renderer::ImportExternalRenderGraphTransientResource(RGResource(OriginalHDRImage), originalImageHandle);
			Renderer::ImportExternalRenderGraphResource(RGResource(HDRCubeImage), m_CubemapImageHandle);
			Renderer::ImportExternalRenderGraphResource(RGResource(IrradianceImage), m_IrradianceImageHandle);
		});
	}
}