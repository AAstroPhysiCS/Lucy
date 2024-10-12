#pragma once

#include "Core/Base.h"

namespace Lucy {

	static constexpr auto ThreadApplicationAffinityIncremental = ~0u;
	static constexpr auto InvalidStatusCode = ~0u;

	enum class ThreadPriority : uint8_t {
		Lowest,
		Normal,
		AboveNormal,
		Highest
	};

	static inline int32_t ConvertToPlatformSpecificPrioritySystem(ThreadPriority priority) {
#ifdef LUCY_WINDOWS
		switch (priority) {
			using enum ThreadPriority;
			case Lowest:
				return THREAD_PRIORITY_LOWEST;
			case Normal:
				return THREAD_PRIORITY_NORMAL;
			case Highest:
				return THREAD_PRIORITY_HIGHEST;
			case AboveNormal:
				return THREAD_PRIORITY_ABOVE_NORMAL;
		}
		return -1;
#endif
	}

#ifdef LUCY_WINDOWS
	static inline void SetThreadArguments(HANDLE handle, const std::string& name, DWORD_PTR affinityMask, uint32_t priorityLevel) {
		DWORD_PTR affinityResult = SetThreadAffinityMask(handle, affinityMask);
		LUCY_ASSERT(affinityResult != 0);

		BOOL priorityResult = SetThreadPriority(handle, priorityLevel);
		LUCY_ASSERT(priorityResult != 0);

		std::wstringstream wss;
		wss << name.c_str();
		HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
		LUCY_ASSERT(SUCCEEDED(hr));
	}
#endif
}