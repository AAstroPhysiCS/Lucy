#pragma once
#include <map>

#include "Material.h"

struct aiMaterial;

namespace Lucy {

	class ImageSampler;
	class PipelineManager;

	class MaterialManager final {
	public:
		MaterialManager(const Unique<PipelineManager>& pipelineManager);
		~MaterialManager() = default;

		MaterialManager(const MaterialManager&) = delete;
		MaterialManager& operator=(const MaterialManager&) = delete;
		MaterialManager(MaterialManager&&) = delete;
		MaterialManager& operator=(MaterialManager&&) = delete;

		RenderDeviceObjectHandle CreateMaterialByPath(MaterialType materialType, aiMaterial* aiMaterial, const std::string& importedFilePath);
		void RTDestroyMaterial(RenderDeviceObjectHandle materialID);
		void RTDestroyMaterials(const std::vector<RenderDeviceObjectHandle>& materialIDs);
		void DestroyAll();

		inline const Ref<Material>& GetMaterialByID(RenderDeviceObjectHandle materialID) const { return m_Materials.at(materialID); }
	private:
		RenderDeviceObjectHandle CreatePBRMaterial(aiMaterial* aiMaterial, const std::string& importedFilePath);

		void LoadMaterialTextures(aiMaterial* aiMaterial, const std::string& importedFilePath, const Ref<Material>& material);

		std::map<RenderDeviceObjectHandle, Ref<Material>> m_Materials;

		const Unique<PipelineManager>& m_PipelineManager; //we need this to create materials bcs it references pipelines
	};
}