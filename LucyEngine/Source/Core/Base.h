#pragma once

#include <memory>
#include <iostream>
#include <functional>

#include "Renderer/Memory/Memory.h"
#include "Renderer/Context/RenderContextResultCodes.h"

#include "Logger.h"
#include "Optick/optick.h"

#define LUCY_WARN(arg) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_WARN, arg)
#define LUCY_CRITICAL(arg) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_CRITICAL, arg)
#define LUCY_INFO(arg) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_INFO, arg)

#ifdef LUCY_WINDOWS
	//for potential platform diversion (in android for example its some asm instruction)
	#define LUCY_DEBUG_BREAK __debugbreak();
#endif

#define LUCY_BIND_FUNC(func) std::bind(func, this, std::placeholders::_1)

#define LUCY_ASSERT(arg)											if(!arg) {																							\
																		LUCY_CRITICAL(fmt::format("Assert failed! File: {0}, Line: {0}", __FILE__, __LINE__));			\
																		LUCY_DEBUG_BREAK }

#define LUCY_ASSERT_TEXT(arg, text)									if(!arg) {																							\
																		LUCY_CRITICAL(text);																			\
																		LUCY_CRITICAL("Assert failed!");																\
																		LUCY_DEBUG_BREAK }

#define LUCY_VK_ASSERT(arg)											if(arg != VK_SUCCESS) {																				\
																		LUCY_CRITICAL(fmt::format("Vulkan error: {0}", RendererAPICodesToString(arg)));					\
																		LUCY_CRITICAL(fmt::format("Vulkan assert failed! File: {0}, Line: {0}", __FILE__, __LINE__));	\
																		LUCY_DEBUG_BREAK }

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

namespace Lucy {

#define USE_COMPUTE_FOR_CUBEMAP_GEN 1

	using EnqueueFunc = std::function<void()>;

	class ContextPipeline;
	class RenderCommand;

	using CommandFunc = std::function<void(void*, Ref<ContextPipeline>, RenderCommand*)>;
}
