#pragma once

#include <chrono>
#include "Base.h"

namespace Lucy {

	enum class TimeUnit {
		Microseconds, Milliseconds, Nanoseconds
	};

	class ScopedTimer {
	public:
		ScopedTimer(const std::string& title, TimeUnit unit)
			: m_Title(title), m_Begin(std::chrono::steady_clock::now()), m_Unit(unit) {
		}

		ScopedTimer(const std::string& title)
			: m_Title(title), m_Begin(std::chrono::steady_clock::now()) {
		}

		~ScopedTimer() {
			switch (m_Unit) {
				case TimeUnit::Microseconds:
					LUCY_INFO(std::format("{0} took {1} us.", m_Title, GetElapsedMicroseconds()));
					break;
				case TimeUnit::Milliseconds:
					LUCY_INFO(std::format("{0} took {1} ms.", m_Title, GetElapsedMilliseconds()));
					break;
				case TimeUnit::Nanoseconds:
					LUCY_INFO(std::format("{0} took {1} ns.", m_Title, GetElapsedNanoseconds()));
					break;
			}
		}
	private:
		inline int64_t GetElapsedMicroseconds() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_Begin).count(); }
		inline int64_t GetElapsedMilliseconds() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_Begin).count(); }
		inline int64_t GetElapsedNanoseconds() { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - m_Begin).count(); }

		std::chrono::steady_clock::time_point m_Begin;
		TimeUnit m_Unit = TimeUnit::Milliseconds;
		std::string m_Title = "Unknown Title";
	};
}