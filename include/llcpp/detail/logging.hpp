#pragma once

#include <string>
#include <functional>

#include "log_line.hpp"
#include "config.hpp"

namespace llcpp::detail::logging {
    struct level {
        enum level_enum : unsigned int {
            trace = 0,
            debug = 1,
            info = 2,
            warn = 3,
            err = 4,
            critical = 5,
        };
    };

    template<typename PrefixTuple, typename Derived, typename Config = config::default_config>
    struct logger_base {
        static_assert(utils::is_specialization_of<PrefixTuple, std::tuple>::value, "PrefixTuple must be a tuple");
        using config_t = Config;

        logger_base(PrefixTuple&& prefix_tuple = {}) : m_prefix_tuple(std::forward<PrefixTuple>(prefix_tuple)) {}
        virtual ~logger_base() {}

        template<typename LogLine, typename... Args>
        void trace(LogLine&& line, Args&&... args) {
            log(level::trace, std::forward<LogLine>(line), std::forward<Args>(args)...);
        }
        template<typename LogLine, typename... Args>
        void debug(LogLine&& line, Args&&... args) {
            log(level::debug, std::forward<LogLine>(line), std::forward<Args>(args)...);
        }
        template<typename LogLine, typename... Args>
        void info(LogLine&& line, Args&&... args) {
            log(level::info, std::forward<LogLine>(line), std::forward<Args>(args)...);
        }
        template<typename LogLine, typename... Args>
        void warn(LogLine&& line, Args&&... args) {
            log(level::warn, std::forward<LogLine>(line), std::forward<Args>(args)...);
        }
        template<typename LogLine, typename... Args>
        void err(LogLine&& line, Args&&... args) {
            log(level::err, std::forward<LogLine>(line), std::forward<Args>(args)...);
        }
        template<typename LogLine, typename... Args>
        void critical(LogLine&& line, Args&&... args) {
            log(level::critical, std::forward<LogLine>(line), std::forward<Args>(args)...);
        }

        void write(const std::uint8_t *data, const std::size_t len) {
            static_cast<Derived*>(this)->write_impl(data,len);
        }
        void line_hint() {
            static_cast<Derived*>(this)->line_hint_impl();
        }

    protected:
        template<std::size_t... I>
        void apply_prefix_tuples(typename level::level_enum level, std::index_sequence<I...>) {
            (std::get<I>(m_prefix_tuple).apply(level, *this), ...);
        }

        template<typename LogLine, typename... Args>
        void log(typename level::level_enum level, LogLine&& line, Args&&... args) {
            static_assert(std::is_base_of_v<log_line::log_line_base, LogLine>, "LogLine type error, did you mean to use \"\"_log?");            
            apply_prefix_tuples(level, std::make_index_sequence<std::tuple_size_v<PrefixTuple>>{});
            using log_line_t = typename LogLine::template log_line_with_config<config_t>::template log_line_with_suffix<'\n'>;
            log_line_t _line;
            _line(*this, args...);
            line_hint();
        }

        //Override these two functions in your derived logger
        void write_impl(const std::uint8_t *data, const std::size_t len) {
        }
        void line_hint_impl() {
        }

        PrefixTuple m_prefix_tuple;
    };

    template<typename PrefixTuple, typename Config = config::default_config>
    struct file_logger : public detail::logging::logger_base<PrefixTuple, file_logger<PrefixTuple, Config>, Config> {
        friend class detail::logging::logger_base<PrefixTuple, file_logger<PrefixTuple, Config>, Config>;

        file_logger(std::string_view path, PrefixTuple&& prefix_tuple = {}) : m_fp(std::fopen(path.data(), "w"), std::fclose),
            logger_base<PrefixTuple, file_logger<PrefixTuple, Config>, Config>(std::forward<PrefixTuple>(prefix_tuple))
        {
            //TODO: Check errors etc...
        }
        file_logger(std::FILE *fp, PrefixTuple&& prefix_tuple = {}) : m_fp(fp, {}),
            logger_base<PrefixTuple, file_logger<PrefixTuple>>(std::forward<PrefixTuple>(prefix_tuple))
        {
            //TODO: Check errors etc...
        }
        ~file_logger() {
            line_hint_impl(true);
        }

        void line_hint_impl(bool force_flush = false) {
            if (force_flush) {
                std::fwrite(m_cache, m_cache_count, 1, m_fp.get());
                m_cache_count = 0;
            }
        }
    protected:
        void write_impl(const std::uint8_t *data, const std::size_t len) { 
            if (len > sizeof(m_cache)) {
                // Flush current cache
                line_hint_impl(true);
                // Write everything else directly
                std::fwrite(data, len, 1, m_fp.get());
                return;
            }  
            if (m_cache_count + len > sizeof(m_cache)) {
                // TODO: Will probably be faster if we fill the cache, flush and then copy the rest.
                line_hint_impl(true);
            }
            std::memcpy(m_cache + m_cache_count, data, len);
            m_cache_count += len;
        }

        std::unique_ptr<std::FILE, std::function<int(std::FILE*)>> m_fp;
        static constexpr std::size_t m_cache_size = 4 * 1024;
        static thread_local std::uint8_t m_cache[m_cache_size];
        static thread_local std::size_t m_cache_count;
    };

    template<typename PrefixTuple, typename Config>
    thread_local std::uint8_t file_logger<PrefixTuple, Config>::m_cache[m_cache_size] = {0};
    template<typename PrefixTuple, typename Config>
    thread_local std::size_t file_logger<PrefixTuple, Config>::m_cache_count = 0;

    //XXX: Hack...
    template<typename PrefixTuple, typename Config = config::default_config>
    struct stdout_logger : public file_logger<PrefixTuple, Config> {
        stdout_logger(PrefixTuple&& prefix_tuple = {}) : 
            file_logger<PrefixTuple>(stdout, std::forward<PrefixTuple>(prefix_tuple))
        {
        }
    };

    template<typename PrefixTuple, typename Config = config::default_config>
    struct vector_logger : public logger_base<PrefixTuple, vector_logger<PrefixTuple>, Config> {

        vector_logger(std::vector<std::uint8_t>& vec, PrefixTuple&& prefix_tuple = {}) : m_vec(vec),
            logger_base<PrefixTuple, vector_logger<PrefixTuple>>(std::forward<PrefixTuple>(prefix_tuple))
        {
        }

        void line_hint_impl() {
        }

        void write_impl(const std::uint8_t *data, const std::size_t len) {    
            m_vec.insert(m_vec.end(), data, data+len);
        }

    private:
        std::vector<std::uint8_t>& m_vec;
    };
}