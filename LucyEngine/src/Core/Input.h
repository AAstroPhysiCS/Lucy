#pragma once

#include "Events/MouseCode.h"
#include "Events/KeyCodes.h"

namespace Lucy {

	class Input {
	public:
		static bool IsMousePressed(MouseCode mouseCode);
		static bool IsKeyPressed(KeyCode mouseCode);
		static bool IsMouseRelease(KeyCode mouseCode);
		static bool IsKeyRelease(KeyCode mouseCode);

		static auto GetMousePosition() {
			struct Size {
				float x;
				float y;
			};
			return Size{ MouseX, MouseY };
		}
	private:
		Input() = delete;

		static float MouseX, MouseY;

		friend class ImGuiLayer;
	};
}

