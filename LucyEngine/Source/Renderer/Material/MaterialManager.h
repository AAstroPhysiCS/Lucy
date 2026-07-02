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

		MaterialID CreateMaterialByPath(MaterialType materialType, aiMaterial* aiMaterial, const std::string& importedFilePath);
		void RTDestroyMaterial(MaterialID materialID);
		void RTDestroyMaterials(const std::vector<MaterialID>& materialIDs);
		void DestroyAll();

		void UpdateMaterialsIfNecessary();
		inline const Ref<Material>& GetMaterialByID(MaterialID materialID) const { return m_Materials.at(materialID); }
	private:
		MaterialID CreatePBRMaterial(aiMaterial* aiMaterial, const std::string& importedFilePath);
		
		void LoadMaterialTextures(aiMaterial* aiMaterial, const std::string& importedFilePath, const Ref<Material>& material);

		std::map<MaterialID, Ref<Material>> m_Materials;
		static inline MaterialIDProvider s_MaterialIDProvider;

		const Unique<PipelineManager>& m_PipelineManager; //we need this to create materials bcs it references pipelines
	};
}