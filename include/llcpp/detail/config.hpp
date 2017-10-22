#pragma once
#include <tuple>
#include "utils.hpp"
#include "terminators.hpp"
#include "log_line.hpp"

namespace llcpp::detail::config {
    struct base_config {
        // void or type to override
        template<typename Config, typename CharT, CharT... Chars>
        using format_parser = format_parser::format_parser<Config, CharT, Chars...>;
        template<typename Config, typename CharT, CharT... Chars>
        using string_format = string_format::string_format<Config, CharT, Chars...>;
        /*
         * 1. Be careful when overriding or adding terminators. The parser makes decision as it passes the format character by character.
         *    Certain terminator combination may lead to unwanted results. For example, using both the 'd' terminator and a new terminator
         *    that expects to end with 'l' will not handle "%lld" correctly.
         * 2. Make sure that the `terminator tuple` or `additional terminators` are std::tuples
         */
        using terminator_tuple = terminators::builtin_terminator_tuple;
        using additional_terminators = std::tuple<>;

    };

    template<typename FormatParser, typename Base>
    struct _config_with_format_parser : public Base {
        using format_parser = FormatParser;
    };
    template<typename StringFormat, typename Base>
    struct _config_with_string_format : public Base {
        using string_format = StringFormat;
    };
    template<typename TerminatorTuple, typename Base>
    struct _config_with_terminator_tuple : public Base {
        using terminator_tuple = TerminatorTuple;
    };
    template<typename AdditionalTerminators, typename Base>
    struct _config_with_additional_terminators : public Base {
        using additional_terminators = AdditionalTerminators;
    };

    template<typename Base = base_config>
    struct config : public Base {
        template<typename FormatParser>
        using config_with_format_parser = config<_config_with_format_parser<FormatParser, config>>;
        template<typename StringFormat>
        using config_with_string_format = config<_config_with_string_format<StringFormat, config>>;
        template<typename TerminatorTuple>
        using config_with_terminator_tuple = config<_config_with_terminator_tuple<TerminatorTuple, config>>;
        template<typename AdditionalTerminators>
        using config_with_additional_terminators = config<_config_with_additional_terminators<AdditionalTerminators, config>>;
    };

    using default_config = config<base_config>;
}