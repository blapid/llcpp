#pragma once

#include "log_line.hpp"
#include "config.hpp"

template<typename CharT, CharT... Chars>
constexpr auto operator"" _log() noexcept
{
    using log_line_t = typename llcpp::detail::log_line::log_line<llcpp::detail::config::default_config, CharT, Chars...>;
    return log_line_t();
}