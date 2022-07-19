#pragma once

#include <memory>
#include <iostream>
#include <functional>

#include "Renderer/Memory/Memory.h"

#include "Logger.h"

#ifdef LUCY_WINDOWS

	#define LUCY_WARN(arg) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_WARN, arg)
	#define LUCY_CRITICAL(arg) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_CRITICAL, arg)
	#define LUCY_INFO(arg) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_INFO, arg)

	//for potential platform diversion (in android for example its some asm instruction)
	#define LUCY_DEBUG_BREAK __debugbreak();

	#define LUCY_ASSERT(arg) if(!arg) { \
										LUCY_CRITICAL("Assert failed!"); \
										LUCY_DEBUG_BREAK }

	#define LUCY_ASSERT_TEXT(arg, text) if(!arg) { \
										LUCY_CRITICAL(text); \
										LUCY_CRITICAL("Assert failed!"); \
										LUCY_DEBUG_BREAK }

	#define LUCY_VK_ASSERT(arg) if(arg != VK_SUCCESS) { \
										LUCY_CRITICAL(fmt::format("Vulkan error: {0}", arg)); \
										LUCY_CRITICAL("Assert failed!"); \
										LUCY_DEBUG_BREAK }
#endif

namespace Lucy {

	using SubmitFunc = std::function<void()>;

	template <typename ... T>
	using RecordFunc = std::function<void(T...)>;
}
