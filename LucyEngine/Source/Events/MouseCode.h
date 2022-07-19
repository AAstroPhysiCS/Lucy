#pragma once

namespace Lucy {

	enum class MouseCode : uint8_t {
		// From imgui.h
		ImGuiMouseButton_Left = 0,
		ImGuiMouseButton_Right = 1,
		ImGuiMouseButton_Middle = 2,
		ImGuiMouseButton_COUNT = 5,

		// From glfw3.h
		Button0 = 0,
		Button1 = 1,
		Button2 = 2,
		Button3 = 3,
		Button4 = 4,
		Button5 = 5,
		Button6 = 6,
		Button7 = 7,

		ButtonLast = Button7,
		ButtonLeft = Button0,
		ButtonRight = Button1,
		ButtonMiddle = Button2
	};
}