#pragma once

#include "Events/MouseCode.h"
#include "Events/KeyCodes.h"

#include "GLFW/glfw3.h"

namespace Lucy {

	class Input {
	public:
		static void Init(GLFWwindow* windowRawPtr);

		static bool IsMousePressed(MouseCode mouseCode);
		static bool IsKeyPressed(KeyCode mouseCode);
		static bool IsMouseRelease(KeyCode mouseCode);
		static bool IsKeyRelease(KeyCode mouseCode);

		static auto GetMousePosition() {
			struct Position {
				double x;
				double y;
			};
			return Position{ MouseX, MouseY };
		}
	private:
		Input() = delete;
		~Input() = delete;
		static double MouseX, MouseY;

		static GLFWwindow* m_RawWindowPtr;

		friend class ImGuiOverlay; //to fetch the newest MouseX and MouseY
	};
}

