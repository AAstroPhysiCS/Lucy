#include "lypch.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Material::Material(const MaterialCreateInfo& createInfo)
		: m_MaterialID(createInfo.MaterialID), m_MaterialType(createInfo.MaterialType), m_Pipeline(createInfo.Pipeline) {
	}

	void Material::AddTexture(size_t pos, RenderResourceHandle textureHandle) {
		m_TextureHandles.insert(m_TextureHandles.begin() + pos, textureHandle);	
	}

	Ref<Image> Lucy::Material::GetImage(const MaterialImageType& type) const {
		return Renderer::AccessResource<Image>(m_TextureHandles[type.Index]);
	}

	bool Material::HasImage(const MaterialImageType& type) const {
		return !m_TextureHandles.empty() &&
			m_TextureHandles.size() > type.Index &&
			m_TextureHandles[type.Index] != InvalidRenderResourceHandle;
	}
}
