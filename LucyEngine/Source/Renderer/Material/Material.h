#pragma once

#include "Utilities/UUID.h"

#include "Renderer/Memory/Memory.h"

namespace Lucy {

	class RenderDevice;

	using MaterialID = float;

	class Material : public MemoryTrackable {
	public:
		Material(MaterialID materialID)
			: m_MaterialID(materialID) {
		}
		virtual ~Material() = default;
		virtual void Update() = 0;
		virtual void RTDestroyResource() = 0;

		inline MaterialID GetMaterialID() const { return m_MaterialID; }
	private:
		MaterialID m_MaterialID = InvalidID<MaterialID>;
	};
}