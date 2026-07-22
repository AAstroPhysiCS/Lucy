#include "lypch.h"
#include "Material.h"

#include "Renderer/Renderer.h"
#include "Renderer/Image/Image.h"

namespace Lucy {

	Material::Material(const MaterialCreateInfo& createInfo)
		: m_MaterialDeviceID(createInfo.MaterialDeviceID), m_MaterialType(createInfo.MaterialType) {
	}

	void Material::AddTexture(size_t pos, RenderDeviceResourceHandle textureHandle) {
		if (m_TextureHandles.size() <= pos)
			m_TextureHandles.resize(pos + 1);
		m_TextureHandles[pos] = textureHandle;
	}

	Ref<Image> Material::GetImage(const MaterialImageType& type) const {
		if (!HasImage(type))
			return nullptr;
		return Renderer::AccessResource<Image>(m_TextureHandles[type.Index]);
	}

	bool Material::HasImage(const MaterialImageType& type) const {
		return !m_TextureHandles.empty() && m_TextureHandles.size() > type.Index && m_TextureHandles[type.Index];
	}
}
 