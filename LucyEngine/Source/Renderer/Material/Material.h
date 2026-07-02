#pragma once

#include "Utilities/UUID.h"

#include "Renderer/Device/RenderResource.h"

namespace Lucy {

	class RenderDevice;
	class Image;
	class Pipeline;

	using MaterialID = float;

	using MaterialIDProvider = IDProvider<MaterialID>;

	enum class MaterialType {
		PBR,
		Subsurface, //TODO:
		Skin, //TODO:
		Hair //TODO:
	};

	struct MaterialCreateInfo {
		MaterialID MaterialID = InvalidID<float>;
		MaterialType MaterialType = MaterialType::PBR;
		Ref<Pipeline> Pipeline = nullptr;
	};

	struct MaterialImageType {
		std::string Name;
		uint32_t Index;
	};

	class Material : public MemoryTrackable {
	public:
		Material(const MaterialCreateInfo& createInfo);
		virtual ~Material() = default;
		virtual void Update() = 0;
		virtual void RTDestroyResource() = 0;

		inline MaterialID GetMaterialID() const { return m_MaterialID; }
		inline MaterialType GetMaterialType() const { return m_MaterialType; }
		inline Ref<Pipeline> GetPipeline() const { return m_Pipeline; }

		virtual void SetTexture(const MaterialImageType& type, RenderResourceHandle textureHandle) = 0;
		
		Ref<Image> GetImage(const MaterialImageType& type) const;
		bool HasImage(const MaterialImageType& type) const;
	protected:
		void AddTexture(size_t pos, RenderResourceHandle textureHandle);

		inline const std::vector<RenderResourceHandle>& GetAllTextureHandles() const { return m_TextureHandles; }
		inline RenderResourceHandle GetTextureHandle(size_t index) const { return m_TextureHandles.at(index); }
	private:
		MaterialID m_MaterialID = InvalidID<MaterialID>;
		MaterialType m_MaterialType = MaterialType::PBR;
		Ref<Pipeline> m_Pipeline = nullptr;

		std::vector<RenderResourceHandle> m_TextureHandles;
	};
}