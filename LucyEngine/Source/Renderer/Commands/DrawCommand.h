#pragma once

#include "Core/Base.h"
#include "../Mesh.h"

namespace Lucy {

	static uint32_t g_Id = 0;

	//Priority for the upcoming job system (multithreading)
	enum class Priority : uint8_t {
		HIGH, LOW
	};

	class DrawCommand {
	public:
		DrawCommand(uint32_t id, Priority priority)
			: m_Id(id), m_Priority(priority) {
		}
		virtual ~DrawCommand() = default;
	protected:
#ifdef LUCY_DEBUG
		std::string m_Name = "Unknown"; //for debugging purposes
#endif
		Priority m_Priority = Priority::LOW; //start with low
		uint32_t m_Id;
	};

	struct MeshDrawCommand : public DrawCommand {
		MeshDrawCommand() = default;
		virtual ~MeshDrawCommand() = default;

		MeshDrawCommand(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform)
			: DrawCommand(g_Id++, priority), Mesh(mesh), EntityTransform(entityTransform) {
#ifdef LUCY_DEBUG
			m_Name = fmt::format("MeshDrawCommand {0}", m_Id);
#endif
		}

		Ref<Mesh> Mesh = nullptr;
		glm::mat4 EntityTransform;
	};
}