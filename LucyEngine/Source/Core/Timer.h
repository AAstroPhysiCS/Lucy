#pragma once

#include <chrono>
#include "Base.h"

namespace Lucy {

	enum class TimeUnit {
		Microseconds, Milliseconds, Nanoseconds
	};

	class ScopedTimer {
	public:
		ScopedTimer(TimeUnit unit)
			: m_Begin(std::chrono::steady_clock::now()), m_Unit(unit) {
		}

		ScopedTimer()
			: m_Begin(std::chrono::steady_clock::now()) {
		}

		~ScopedTimer() {
			switch (m_Unit) {
			case TimeUnit::Microseconds:
				LUCY_INFO(fmt::format("Took {0} us.", GetElapsedMicroseconds()));
				break;
			case TimeUnit::Milliseconds:
				LUCY_INFO(fmt::format("Took {0} ms.", GetElapsedMilliseconds()));
				break;
			case TimeUnit::Nanoseconds:
				LUCY_INFO(fmt::format("Took {0} ns.", GetElapsedNanoseconds()));
				break;
			}
		}
	private:
		inline int64_t GetElapsedMicroseconds() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_Begin).count(); }
		inline int64_t GetElapsedMilliseconds() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_Begin).count(); }
		inline int64_t GetElapsedNanoseconds() { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - m_Begin).count(); }

		std::chrono::steady_clock::time_point m_Begin;
		TimeUnit m_Unit = TimeUnit::Milliseconds;
	};
}