#pragma once

#include <format>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Lucy {

	//https://en.cppreference.com/w/cpp/utility/format/formattable
	template <typename T>
	concept IsFormattable = requires (T&& v, std::format_context ctx) {
		std::formatter<std::remove_cvref_t<T>>().format(v, ctx);
	};

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
		static inline std::shared_ptr<spdlog::logger> s_Logger;

		static void Init() {
			s_Logger = spdlog::stdout_color_mt("LUCY");
			s_Logger->set_pattern("%^[%T] %n: %v%$");
			s_Logger->set_level((spdlog::level::level_enum)LoggerInfo::LUCY_TRACE);
		}

		template <IsFormattable T, typename ... TArgs>
		static void Log(LoggerInfo info, T&& log, TArgs&& ... args) {
			s_Logger->set_level((spdlog::level::level_enum)info);
			auto&& formattedT = std::vformat(std::forward<T>(log), std::make_format_args(args...));
			switch (info) {
				using enum Lucy::LoggerInfo;
				case LUCY_WARN:
					s_Logger->warn(formattedT);
					break;
				case LUCY_INFO:
					s_Logger->info(formattedT);
					break;
				case LUCY_CRITICAL:
					s_Logger->critical(formattedT);
					break;
				case LUCY_TRACE:
					s_Logger->trace(formattedT);
					break;
				case LUCY_LDEBUG:
					s_Logger->debug(formattedT);
					break;
				case LUCY_ERROR:
					s_Logger->error(formattedT);
					break;
				default:
					s_Logger->error("Unhandled logger message!");
					break;
			}
		}
	private:
		Logger() = delete;
		~Logger() = delete;
	};
}