#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Lucy {

	enum class LoggerInfo {
		LUCY_TRACE,
		LUCY_LDEBUG,
		LUCY_INFO,
		LUCY_WARN,
		LUCY_ERROR,
		LUCY_CRITICAL,
		LUCY_OFF,
		LUCY_NLEVELS
	};

	class Logger {
	public:
		inline static std::shared_ptr<spdlog::logger> s_Logger;

		static void Init() {
			s_Logger = spdlog::stdout_color_mt("LUCY");
			s_Logger->set_pattern("%^[%T] %n: %v%$");
			s_Logger->set_level((spdlog::level::level_enum)LoggerInfo::LUCY_TRACE);
		}

		template<typename T>
		static void Log(LoggerInfo info, T&& log) {
			switch (info) {
			case LoggerInfo::LUCY_WARN:
				s_Logger->set_level((spdlog::level::level_enum)info);
				s_Logger->warn(log);
				break;
			case LoggerInfo::LUCY_INFO:
				s_Logger->set_level((spdlog::level::level_enum)info);
				s_Logger->info(log);
				break;
			case LoggerInfo::LUCY_CRITICAL:
				s_Logger->set_level((spdlog::level::level_enum)info);
				s_Logger->critical(log);
				break;
			}
		}

		template<typename T>
		static void LogInfo(T&& log) {
			s_Logger->set_level((spdlog::level::level_enum)LoggerInfo::LUCY_INFO);
			s_Logger->info(log);
		}

		template<typename T>
		static void LogCritical(T&& log) {
			s_Logger->set_level((spdlog::level::level_enum)LoggerInfo::LUCY_CRITICAL);
			s_Logger->critical(log);
		}

		template<typename T>
		static void LogWarning(T&& log) {
			s_Logger->set_level((spdlog::level::level_enum)LoggerInfo::LUCY_WARN);
			s_Logger->warn(log);
		}

	private:
		Logger() = delete;
		~Logger() = delete;
	};
}