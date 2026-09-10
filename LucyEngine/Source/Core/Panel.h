#pragma once

#include "Core/Base.h" //for child classes

#include "Events/KeyCodes.h" //for child classes
#include "Events/MouseCode.h" //for child classes
#include "Events/Event.h"

#include "imgui.h"

namespace Lucy {

	namespace Theme {
		inline constexpr ImVec4 BackgroundColor = { 0.045f, 0.050f, 0.060f, 1.0f };
		inline constexpr ImVec4 ChildBackgroundColor = { 0.065f, 0.070f, 0.085f, 1.0f };
		inline constexpr ImVec4 HeaderColor = { 0.090f, 0.105f, 0.130f, 1.0f };
		inline constexpr ImVec4 HeaderHoveredColor = { 0.120f, 0.145f, 0.180f, 1.0f };
		inline constexpr ImVec4 AccentColor = { 0.250f, 0.550f, 1.000f, 1.0f };
		inline constexpr ImVec4 GreenColor = { 0.350f, 0.850f, 0.500f, 1.0f };
		inline constexpr ImVec4 YellowColor = { 1.000f, 0.720f, 0.250f, 1.0f };
		inline constexpr ImVec4 RedColor = { 1.000f, 0.350f, 0.350f, 1.0f };
	}

	class Panel {
	public:
		Panel() = default;
		virtual ~Panel() = default;

		Panel(const Panel&) = delete;
		Panel& operator=(const Panel&) = delete;
		Panel(Panel&&) = delete;
		Panel& operator=(Panel&&) = delete;

		virtual void OnEvent(Event& e) { /* Could be overriden by the corresponding child class */ }
		virtual void OnDestroy() { /* Could be overriden by the corresponding child class */ }
		virtual void Render() = 0;

		inline void ToggleShow() { m_Show = !m_Show; }
		inline bool GetShow() const { return m_Show; }
	private:
		bool m_Show = true;
	};
}