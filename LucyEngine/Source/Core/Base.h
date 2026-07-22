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

#define TRACY_ON_DEMAND
#include "tracy/Tracy.hpp"

#include "glm/common.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/integer.hpp"

#define LUCY_WARN(arg, ...) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_WARN, arg, __VA_ARGS__)
#define LUCY_CRITICAL(arg, ...) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_CRITICAL, arg, __VA_ARGS__)
#define LUCY_INFO(arg, ...) Lucy::Logger::Log(Lucy::LoggerInfo::LUCY_INFO, arg, __VA_ARGS__)

#ifdef LUCY_WINDOWS
	#define LUCY_DEBUG_BREAK __debugbreak()
#endif

#define LUCY_BIND_FUNC(func, self, ...) std::bind(func, self, __VA_ARGS__)

#ifdef LUCY_DEBUG
    #define LUCY_ENABLE_ASSERTS 1
#else
    #define LUCY_ENABLE_ASSERTS 0
#endif

static void AppendLocationInfo(std::string& message, const std::source_location& location) noexcept {
    message += "\nFile: ";
    message += location.file_name();
    message += "\nLine: ";
    message += std::to_string(location.line());
    message += "\nFunc: ";
    message += location.function_name();
}

static void HandleAssertFailure(const std::source_location& location, std::string_view message) {
    std::string fullMessage(message);
    fullMessage.reserve(256);  // Preallocate to avoid reallocations
    AppendLocationInfo(fullMessage, location);
    LUCY_CRITICAL(fullMessage);
    LUCY_DEBUG_BREAK;
}

#if LUCY_ENABLE_ASSERTS
    static void LucyAssert(bool condition, const std::source_location& location) {
        if (!condition) [[unlikely]] {
            HandleAssertFailure(location, "Assertion failed");
        }
    }

    template <typename... Args>
    static void LucyAssert(bool condition, const std::source_location& location,
        std::string_view format, Args&&... args) {
        if (!condition) [[unlikely]] {
            std::string message = std::vformat(format, std::make_format_args(args...));
            HandleAssertFailure(location, message);
        }
    }

    template <typename T>
    static void LucyAssert(Lucy::Ref<T> ref, const std::source_location& location) {
        if (!ref) [[unlikely]] {
            HandleAssertFailure(location, "Ref assertion failed");
        }
    }

    template <typename T>
    static void LucyAssert(Lucy::Ref<T> ref, const std::source_location& location,
        std::string_view message) {
        if (!ref) [[unlikely]] {
            HandleAssertFailure(location, message);
        }
    }
#else
    static void LucyAssert(bool, const std::source_location&) noexcept {}
    template <typename... Args>
    static void LucyAssert(bool, const std::source_location&, std::string_view, Args&&...) noexcept {}
    template <typename T>
    static void LucyAssert(Lucy::Ref<T>, const std::source_location&) noexcept {}
    template <typename T>
    static void LucyAssert(Lucy::Ref<T>, const std::source_location&, std::string_view) noexcept {}
#endif

static void LucyVulkanAssert(int32_t result, const std::source_location& location) {
    if (result != 0) [[unlikely]] {
        std::string message = "Vulkan error: ";
        message += Lucy::RendererBackendCodesToString(result);
        HandleAssertFailure(location, message);
    }
}

#define LUCY_ASSERT(condition, ...) \
    LucyAssert((condition), std::source_location::current(), ##__VA_ARGS__)

#define LUCY_VK_ASSERT(result) \
    LucyVulkanAssert((result), std::source_location::current())

#define LUCY_PROFILE_NEW_FRAME(Name)								FrameMarkNamed(Name)
#define LUCY_PROFILE_NEW_THREAD(Name)								(void)0;
#define LUCY_PROFILE_NEW_EVENT(Name)								ZoneScopedN(Name)
#define LUCY_PROFILE_DESTROY()										(void)0;

#define IMGUI_DEFINE_MATH_OPERATORS

#define USE_COMPUTE_FOR_CUBEMAP_GEN 1

#define USE_INTEGRATED_GRAPHICS 0