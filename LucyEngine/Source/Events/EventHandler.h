#pragma once

#include <mutex>

#include "Core/Base.h"

#include "MouseCode.h"
#include "KeyCodes.h"

#include "InputEvent.h"
#include "WindowEvent.h"

#include "Event.h"

struct GLFWwindow;

namespace Lucy {

	template <typename TEvent>
	concept IsEvent =
		std::derived_from<TEvent, Event> &&
		requires(const TEvent& evt) {
			{ evt.GetEventType() } -> std::same_as<EventType>;
			{ TEvent::EventType } -> std::convertible_to<EventType>;
	};

	class EventQueue final {
	public:
		template <typename TEvent, typename... TArgs>
		void Push(TArgs&&... args) {
			std::scoped_lock lock(m_Mutex);

			m_Events.emplace_back(
				std::make_unique<TEvent>(std::forward<TArgs>(args)...)
			);
		}

		void Push(std::unique_ptr<Event> event);

		template <typename TFunc>
		void Drain(TFunc&& func) {
			std::vector<std::unique_ptr<Event>> events;

			{
				std::scoped_lock lock(m_Mutex);
				events = std::move(m_Events);
				m_Events.clear();
			}

			for (auto& event : events) {
				func(*event);
			}
		}

		bool Empty() const;
	private:
		mutable std::mutex m_Mutex;
		std::vector<std::unique_ptr<Event>> m_Events;
	};

	class EventHandler final {
	public:
		~EventHandler() = default;

		template <typename TEvent, typename... TArgs>
		static void Submit(TArgs&&... args) {
			s_EventQueue->Push<TEvent>(std::forward<TArgs>(args)...);
		}

		template <IsEvent TEvent, typename TFunc>
		static bool AddListener(Event& event, TFunc&& func) {
			if (event.GetEventType() != TEvent::EventType)
				return false;

			TEvent& typedEvent = static_cast<TEvent&>(event);

			//false = this listener did not handle the event
			//true = this listener handled the event
			using ResultType = std::invoke_result_t<TFunc, TEvent&>;

			if constexpr (std::same_as<ResultType, bool>) {
				return std::invoke(std::forward<TFunc>(func), typedEvent);
			} else {
				std::invoke(std::forward<TFunc>(func), typedEvent);
				return true;
			}
		}
	private:
		EventHandler() = default;

		static inline Unique<EventQueue> s_EventQueue = Memory::CreateUnique<EventQueue>();

		friend class Application; // for s_EventQueue / Drain
	};

	struct Input final {
		static bool IsMousePressed(MouseCode mouseCode);
		static bool IsKeyPressed(KeyCode keyCode);

		static bool IsMouseRelease(MouseCode mouseCode);
		static bool IsKeyRelease(KeyCode keyCode);

		static float GetMouseX() { return MouseX; }
		static float GetMouseY() { return MouseY; }

		static void Init(GLFWwindow* window);
	private:
		Input() = default;
		~Input() = default;

		static inline float MouseX = 0.0f;
		static inline float MouseY = 0.0f;

		static inline GLFWwindow* s_RawWindow = nullptr;

		friend class RenderPipeline;
	};
}