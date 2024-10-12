#pragma once

#include <variant>
#include <functional>

#include "MouseCode.h"
#include "KeyCodes.h"

#include "InputEvent.h"
#include "WindowEvent.h"

#include "Core/Application.h"

namespace Lucy {

	template <typename TEvent>
	concept IsEvent = requires(TEvent&& evt) {
		{ evt.GetEventType() } -> std::same_as<EventType>;
	};

	class EventHandler final {
	public:
		template <typename TEvent>
		static inline void AddListener(const Event& evt, const std::function<void(const TEvent&)>& evtFunc) {
			if (evt.GetEventType() == TEvent::EventType)
				evtFunc((const TEvent&)evt);
		}

		template <typename TEvent, typename ... TEventArgs>
		static inline void DispatchImmediateEvent(TEventArgs&& ... args) {
			TEvent evt(std::forward<TEventArgs>(args)...);
			std::invoke(&Application::OnEvent, *s_RawAppPtr, evt);
		}
	private:
		EventHandler() = default;
		~EventHandler() = default;

		static void Init(Application* appPtr, GLFWwindow* windowPtr);

		friend class Application;
		friend struct Input;

		static inline GLFWwindow* s_RawWindowPtr = nullptr;
		static inline Application* s_RawAppPtr = nullptr;
	};

	struct Input final {
		static bool IsMousePressed(MouseCode mouseCode);
		static bool IsKeyPressed(KeyCode mouseCode);
		static bool IsMouseRelease(KeyCode mouseCode);
		static bool IsKeyRelease(KeyCode mouseCode);

		static float GetMouseX() { return MouseX; }
		static float GetMouseY() { return MouseY; }
	private:
		static inline float MouseX = 0;
		static inline float MouseY = 0;

		friend class RenderPipeline;
	};
}