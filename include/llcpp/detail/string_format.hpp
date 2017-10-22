#pragma once

#include "format_parser.hpp"

namespace llcpp::detail::string_format {
    template<typename Config, typename CharT, CharT... Chars>
    struct string_format {
        using config_t = Config;
        using format_parser = typename config_t::template format_parser<config_t, CharT, Chars...>;
        constexpr string_format() {}

        inline static constexpr std::size_t size() {
            return _size;
        }

        inline static constexpr std::size_t fmt_size() {
            return _fmt_size;
        }
        inline static constexpr std::size_t args_size() {
            return _args_size;
        }

        static constexpr std::size_t _fmt_size = sizeof...(Chars) + 1;
        static constexpr std::size_t _args_size = format_parser::sum_arguments_size::value;
        static constexpr std::size_t _size = _fmt_size + _args_size;

        inline static constexpr CharT  _chars[string_format::fmt_size()] = {Chars..., '\0'};
    };
}