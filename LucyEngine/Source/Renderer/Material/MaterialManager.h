#pragma once
#include <map>

#include "Material.h"
#include "Utilities/UUID.h"

struct aiMaterial;

namespace Lucy {

	enum class MaterialType {
		PBR,
		Subsurface, //TODO:
		Skin, //TODO:
		Hair //TODO:
	};

	using MaterialIDProvider = IDProvider<float>;

	class MaterialManager final {
	public:
		MaterialManager(const std::unordered_map<std::string, Ref<Shader>>& shaders);
		~MaterialManager() = default;

		MaterialID CreateMaterialByPath(MaterialType materialType, aiMaterial* aiMaterial, const std::string& importedFilePath);
		void RTDestroyMaterial(MaterialID materialID);
		void RTDestroyMaterials(const std::vector<MaterialID>& materialIDs);
		void DestroyAll();

		void UpdateMaterialsIfNecessary();
		inline const Ref<Material>& GetMaterialByID(MaterialID materialID) const { return m_Materials.at(materialID); }
	private:
		MaterialID CreatePBRMaterial(aiMaterial* aiMaterial, const std::string& importedFilePath);

		std::map<MaterialID, Ref<Material>> m_Materials;
		static inline MaterialIDProvider s_MaterialIDProvider;

		const std::unordered_map<std::string, Ref<Shader>>& m_Shaders;
	};
}