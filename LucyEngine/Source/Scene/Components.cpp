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
		m_Path = path;

		Renderer::EnqueueToRenderCommandQueue([&, path](const Ref<RenderDevice>& device) {
			ImageCreateInfo irradianceImageCreateInfo = {
				.Width = CubemapPass::HDRImageSize,
				.Height = CubemapPass::HDRImageSize,
				.ImageType = ImageType::TypeCube,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Format = ImageFormat::R16G16B16A16_SFLOAT,
				.GenerateSampler = true,
				.GenerateMipmap = MipmapCreateInfo::NoMipmap(),
				.ImGuiUsage = false
			};

			ImageCreateInfo originalImageCreateInfo{
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorTransferAttachment,
				.Format = ImageFormat::R32G32B32A32_SFLOAT,
				.GenerateSampler = true
			};

			m_OriginalImageHandle = device->CreateImage(path, originalImageCreateInfo);

			Renderer::ImportExternalRenderGraphTransientResource(RGResource(OriginalHDRImage), m_OriginalImageHandle);
#if USE_COMPUTE_FOR_CUBEMAP_GEN
			m_IrradianceImageHandle = device->CreateImage(irradianceImageCreateInfo);
			Renderer::ImportExternalRenderGraphResource(RGResource(IrradianceImage), m_IrradianceImageHandle);
#endif
		});
	}
}