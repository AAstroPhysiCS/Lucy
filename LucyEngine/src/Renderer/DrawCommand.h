#pragma once

#include "../Core/Base.h"
#include "Mesh.h"

namespace Lucy {

	struct MeshDrawCommand {
		MeshDrawCommand() = default;
		MeshDrawCommand(RefLucy<Mesh> mesh, const glm::mat4& entityTransform)
			: Mesh(mesh), EntityTransform(entityTransform) {}

		friend class Renderer;
	private:
		RefLucy<Mesh> Mesh;
		glm::mat4 EntityTransform;
	};
}