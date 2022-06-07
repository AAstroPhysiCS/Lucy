#pragma once

#include "../Core/Base.h"
#include "Mesh.h"

namespace Lucy {

	struct MeshDrawCommand {
		MeshDrawCommand() = default;
		~MeshDrawCommand() = default;
		MeshDrawCommand(RefLucy<Mesh> mesh, const glm::mat4& entityTransform)
			: Mesh(mesh), EntityTransform(entityTransform) {
		}

		RefLucy<Mesh> Mesh = nullptr;
		glm::mat4 EntityTransform;
	};
}