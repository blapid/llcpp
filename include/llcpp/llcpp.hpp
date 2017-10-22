#pragma once

#include "detail/config.hpp"
#include "detail/prefix.hpp"
#include "detail/logging.hpp"

namespace llcpp {
    using default_config = detail::config::default_config;

    template<typename PrefixTuple, typename Derived, typename Config = default_config>
    using logger_base = detail::logging::logger_base<PrefixTuple, Derived, Config>;
    template<typename PrefixTuple, typename Config = default_config>
    using file_logger = detail::logging::file_logger<PrefixTuple, Config>;
    template<typename PrefixTuple, typename Config = default_config>
    using stdout_logger = detail::logging::stdout_logger<PrefixTuple, Config>;
    template<typename PrefixTuple, typename Config = default_config>
    using vector_logger = detail::logging::vector_logger<PrefixTuple, Config>;

    using prefix_base = detail::prefix::prefix_base;

    using gmtime_prefix = detail::prefix::time_format_prefix<false>;
    using localtime_prefix = detail::prefix::time_format_prefix<>;
    template<bool UseLocalTime = true>
    using time_format_prefix = detail::prefix::time_format_prefix<UseLocalTime>;
    using log_level_prefix = detail::prefix::log_level_prefix;
    using nanosec_time_prefix = detail::prefix::nanosec_time_prefix;

    using default_logger = file_logger<std::tuple<log_level_prefix, time_format_prefix<true>>>;

    using builtin_terminator_tuple = detail::terminators::builtin_terminator_tuple;


}