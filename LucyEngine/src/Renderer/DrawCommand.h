#pragma once

#include "../Core/Base.h"
#include "Mesh.h"

namespace Lucy {

	struct MeshDrawCommand {
		MeshDrawCommand() = default;
		~MeshDrawCommand() = default;
		MeshDrawCommand(Ref<Mesh> mesh, const glm::mat4& entityTransform)
			: Mesh(mesh), EntityTransform(entityTransform) {
		}

		Ref<Mesh> Mesh = nullptr;
		glm::mat4 EntityTransform;
	};
}