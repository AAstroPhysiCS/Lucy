#include "lypch.h"
#include "Components.h"

#include "Entity.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/RenderDeviceScene.h"
#include "Renderer/RenderGraph/RenderGraphResource.h"
#include "Renderer/RendererPasses.h"

#include "Renderer/Image/Image.h"

#include "SceneImporter.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace Lucy {

	TransformComponent::TransformComponent(const glm::mat4& mat) 
		: m_Mat(mat) {
		glm::quat orientation{};
		glm::vec3 skew{};
		glm::vec4 perspective{};

		glm::decompose(m_Mat, m_Scale, orientation, m_Position, skew, perspective);

		orientation = glm::normalize(orientation);
		m_Rotation = glm::degrees(glm::eulerAngles(orientation));
	}

	void TransformComponent::CalculateMatrix() {
		m_Mat = glm::translate(glm::mat4(1.0f), m_Position)
			* glm::toMat4(glm::quat(glm::radians(m_Rotation)))
			* glm::scale(glm::mat4(1.0f), m_Scale);
	}

	void MeshComponent::LoadMesh(const Entity& e, const std::string& path) {
		m_Mesh = std::move(Memory::CreateRef<Mesh>(path));

		Renderer::EnqueueToRenderCommandQueue([this, e](const Ref<RenderDevice>& device) mutable {
			const auto& transform = e.GetComponent<TransformComponent>().GetMatrix();
			m_Handle = device->GetScene()->RTRegisterObject(m_Mesh->GetRenderDeviceMeshHandle(), transform, RenderDeviceObjectFlags::None);
		});
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

	CameraComponent::CameraComponent(const ImportedCamera& importedCamera) 
		: m_Camera(importedCamera.Position, importedCamera.NearPlane, importedCamera.FarPlane, importedCamera.VerticalFOV) {
		m_Camera.SetAspectRatio(importedCamera.AspectRatio);
	}

	PunctualLightComponent::PunctualLightComponent(const ImportedLight& light)
		: m_Color(light.Color), m_Range(light.Range), m_InnerConeAngle(light.InnerConeAngle), m_OuterConeAngle(light.OuterConeAngle) {
		switch (light.Type) {
			case ImportedLightType::Point:
				m_Type = PunctualLightType::Point;
				break;
			case ImportedLightType::Spot:
				m_Type = PunctualLightType::Spot;
				break;
			default:
				LUCY_ASSERT(false, "Unsupported punctual light type!");
				break;
		}

		m_Intensity = glm::max(light.Color.r, glm::max( light.Color.g, light.Color.b));
		if (m_Intensity > 0.0f)
			m_Color = light.Color / m_Intensity;
	}

	PunctualLightComponent::PunctualLightComponent(PunctualLightType type) 
		: m_Type(type) {
	}
}