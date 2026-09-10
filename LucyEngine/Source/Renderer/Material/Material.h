#pragma once

#include <any>

#include "Renderer/Device/RenderDeviceResource.h"
#include "Renderer/Device/RenderDeviceHandles.h"

namespace Lucy {

	class RenderDevice;
	class Image;
	class Pipeline;

	enum class MaterialType {
		PBR,
		Subsurface, //TODO:
		Skin, //TODO:
		Hair //TODO:
	};

	struct MaterialCreateInfo {
		RenderDeviceObjectHandle MaterialDeviceID{};
		MaterialType MaterialType = MaterialType::PBR;
	};

	struct MaterialImageType {
		std::string Name;
		uint32_t Index;
	};

	class Material : public MemoryTrackable {
	public:
		Material(const MaterialCreateInfo& createInfo);
		virtual ~Material() = default;

		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;
		Material(Material&&) = delete;
		Material& operator=(Material&&) = delete;

		virtual std::any BuildRenderData(const Ref<RenderDevice>& device) = 0;
		virtual void RTDestroyResource() = 0;

		inline RenderDeviceObjectHandle GetMaterialDeviceID() const { return m_MaterialDeviceID; }
		inline RenderDeviceObjectHandle& GetMaterialDeviceID() { return m_MaterialDeviceID; }

		inline MaterialType GetMaterialType() const { return m_MaterialType; }

		virtual void SetTexture(const MaterialImageType& type, RenderDeviceResourceHandle textureHandle) = 0;
		
		Ref<Image> GetImage(const MaterialImageType& type) const;
		bool HasImage(const MaterialImageType& type) const;
	protected:
		void AddTexture(size_t pos, RenderDeviceResourceHandle textureHandle);

		inline std::vector<RenderDeviceResourceHandle>& GetAllTextureHandles() { return m_TextureHandles; }
		inline const std::vector<RenderDeviceResourceHandle>& GetAllTextureHandles() const { return m_TextureHandles; }
		inline RenderDeviceResourceHandle GetTextureHandle(size_t index) const { return m_TextureHandles.at(index); }
	private:
		RenderDeviceObjectHandle m_MaterialDeviceID{};
		MaterialType m_MaterialType = MaterialType::PBR;

		std::vector<RenderDeviceResourceHandle> m_TextureHandles;
	};
}