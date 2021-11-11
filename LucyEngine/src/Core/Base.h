#pragma once

#include <memory>
#include <iostream>
#include <vector>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <any>
#include <functional>
#include <algorithm>

#include "Logger.h"
#include "Application.h"

#ifdef LUCY_WINDOWS

#define LUCY_WARN(arg) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_WARN, arg)
#define LUCY_CRITICAL(arg) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_CRITICAL, arg)
#define LUCY_INFO(arg) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_INFO, arg)

#ifdef LUCY_DEBUG
//for potential platform diversion (in android for example its some asm instruction)
#define LUCY_DEBUG_BREAK __debugbreak();

#define LUCY_ASSERT(arg) if(!arg) { \
							LUCY_CRITICAL("Assert failed!"); \
							LUCY_DEBUG_BREAK }
#elif LUCY_RELEASE
#define LUCY_ASSERT(arg)
#endif
#endif

#define As(arg, T) std::static_pointer_cast<T>(arg)

namespace Lucy {

	using Func = std::function<void()>;

	template<typename T>
	using RefLucy = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	inline RefLucy<T> CreateRef(Args&& ... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using ScopeLucy = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	inline ScopeLucy<T> CreateScope(Args&& ... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
}
