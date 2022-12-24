#pragma once

#include "Events/MouseCode.h"
#include "Events/KeyCodes.h"

#include "Events/Event.h"

#include "GLFW/glfw3.h"

namespace Lucy {

	class InputHandler {
	private:
		InputHandler() = default;
		~InputHandler() = default;

		friend class Application;
	public:
		void Init(GLFWwindow* windowRawPtr);

		template <typename T>
		inline void Dispatch(Event& evt, EventType type, std::function<void(T&)>&& fnc) {
			if (evt.GetType() == type) {
				fnc((T&)evt);
			}
		}

		bool IsMousePressed(MouseCode mouseCode);
		bool IsKeyPressed(KeyCode mouseCode);
		bool IsMouseRelease(KeyCode mouseCode);
		bool IsKeyRelease(KeyCode mouseCode);

		auto GetMousePosition() {
			struct Position {
				double x;
				double y;
			};
			return Position{ MouseX, MouseY };
		}
	private:
		double MouseX, MouseY;
		GLFWwindow* m_RawWindowPtr;

		friend class ImGuiOverlay; //to fetch the newest MouseX and MouseY
	};
}

