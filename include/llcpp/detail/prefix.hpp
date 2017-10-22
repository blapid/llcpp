#pragma once

#include <chrono>
#include <array>

#include "udl.hpp"
#include "logging.hpp"

namespace llcpp::detail::prefix {

    struct prefix_base {
        virtual ~prefix_base() {}

        template<typename Logger>
        inline void apply(logging::level::level_enum level, Logger& logger) {}
    };

    template<bool UseLocalTime = true>
    struct time_format_prefix : public prefix_base {
        inline std::tm localtime()
        {
            //TODO check errors
            std::time_t now_t = std::time(nullptr);
            std::tm tm;
            if constexpr (UseLocalTime) {
                localtime_r(&now_t, &tm);
            } else {
                gmtime_r(&now_t, &tm);
            }
            
            return tm;
        }

        template<typename Logger>
        inline void apply(typename logging::level::level_enum level, Logger& logger) {
            auto lt = localtime();
            "[%d-%3s-%d %d:%d:%d]: "_log(logger, lt.tm_year + 1900, m_month_strings[lt.tm_mon], lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
        }

        inline static constexpr std::array<const char *, 12> m_month_strings = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
    };
    struct nanosec_time_prefix : public prefix_base {
        template<typename Logger>
        inline void apply(typename logging::level::level_enum level, Logger& logger) {
            auto now = std::chrono::system_clock::now();
            auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
            "[%llu]: "_log(logger, now_ns);
        }
    };
    struct log_level_prefix : public prefix_base {

        template<typename Logger>        
        inline void apply(typename logging::level::level_enum level, Logger& logger) {
            "[%8s]"_log(logger, m_level_strings[level]);
        }

        inline static constexpr std::array<const char *, 6> m_level_strings = {
            "TRACE",
            "DEBUG",
            "INFO",
            "WARN",
            "ERR",
            "CRITICAL"
        };
    };
}