#pragma once

#include <memory>
#include <iostream>
#include <functional>
#include <source_location>
#include <format>
#include <filesystem>

#include "Renderer/Memory/Memory.h"
#include "Renderer/Context/RenderContextResultCodes.h"

#include "Logger.h"
#include "Optick/optick.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include "glm/common.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/integer.hpp"

#define LUCY_WARN(arg, ...) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_WARN, arg, __VA_ARGS__)
#define LUCY_CRITICAL(arg, ...) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_CRITICAL, arg, __VA_ARGS__)
#define LUCY_INFO(arg, ...) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_INFO, arg, __VA_ARGS__)

#ifdef LUCY_WINDOWS
	//for potential platform diversion (in android for example its some asm instruction)
	#define LUCY_DEBUG_BREAK __debugbreak()
#endif

#define LUCY_BIND_FUNC(func, self, ...) std::bind(func, self, __VA_ARGS__)

static std::string LucyGetFunctionStack(const std::source_location& location, const std::string& msg) {
	return std::format("{}\nFile: {}, Line: {}, Column: {}, Function name: {}", msg,
					   location.file_name(), location.line(), location.column(), location.function_name());
}

template <typename... T>
static void LucyAssert(bool arg, const std::source_location& location, const std::string& text = "", T&&... args) {
	if (!arg) {
		LUCY_CRITICAL(LucyGetFunctionStack(location, std::vformat(text, std::make_format_args(args...))));
		LUCY_DEBUG_BREAK;
	}
}

template <typename T>
static void LucyAssert(Lucy::Ref<T> arg, const std::source_location& location, const std::string& text = "") {
	if (!arg) {
		LUCY_CRITICAL(LucyGetFunctionStack(location, text));
		LUCY_DEBUG_BREAK;
	}
}

static void LucyVulkanAssert(int32_t result, const std::source_location& location) {
	if (result != 0) //0 for VK_SUCCESS
		LucyAssert(false, location, "Vulkan error: {0}", Lucy::RendererBackendCodesToString(result));
}

#define NUMARGS(...)												std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value

#define LUCY_ASSERT(arg, ...)										if (NUMARGS(__VA_ARGS__) == 0) LucyAssert(arg, std::source_location::current()); else LucyAssert((arg), std::source_location::current(), __VA_ARGS__)
#define LUCY_VK_ASSERT(arg)											LucyVulkanAssert(arg, std::source_location::current())

#ifdef LUCY_DEBUG
	#define USE_OPTICK (1)
#else
	#define USE_OPTICK (0)
#endif

#define LUCY_PROFILE_NEW_FRAME(Name)								OPTICK_FRAME(Name)
#define LUCY_PROFILE_NEW_THREAD(Name)								OPTICK_THREAD(Name)
#define LUCY_PROFILE_NEW_EVENT(Name)								OPTICK_EVENT(Name)
#define LUCY_PROFILE_NEW_TAG(Name, ...)								OPTICK_TAG(Name, __VA_ARGS__)
#define LUCY_PROFILE_NEW_CATEGORY(Name, Category)					OPTICK_CATEGORY(Name, Category)
#define LUCY_PROFILE_DESTROY()										OPTICK_SHUTDOWN()

//not using it, since i will create my own
#if defined (LUCY_PROFILE_GPU_OPTICK) && defined (LUCY_DEBUG)
	#define LUCY_PROFILE_GPU_INIT(...)								Optick::InitGpuVulkan(__VA_ARGS__)
	#define LUCY_PROFILE_GPU_EVENT(Name)							OPTICK_GPU_EVENT(Name)
	#define LUCY_PROFILE_GPU_FLIP(SwapChainHandle)					OPTICK_GPU_FLIP(SwapChainHandle)
	#define LUCY_PROFILE_GPU_CONTEXT(...)							OPTICK_GPU_CONTEXT(__VA_ARGS__)
#endif

#define USE_COMPUTE_FOR_CUBEMAP_GEN 1