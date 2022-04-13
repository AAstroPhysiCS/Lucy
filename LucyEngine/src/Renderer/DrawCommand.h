#pragma once

#include "../Core/Base.h"
#include "Mesh.h"

namespace Lucy {

	struct MeshDrawCommand {
		MeshDrawCommand() = default;
		MeshDrawCommand(RefLucy<Pipeline> pipeline, RefLucy<Mesh> mesh, const glm::mat4& entityTransform)
			: Pipeline(pipeline), Mesh(mesh), EntityTransform(entityTransform) {
		}

		RefLucy<Pipeline> Pipeline = nullptr;
		RefLucy<Mesh> Mesh = nullptr;
		glm::mat4 EntityTransform;
	};
}