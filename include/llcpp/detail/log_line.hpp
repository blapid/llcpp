#pragma once

#include "string_format.hpp"
#include "format_parser.hpp"
#include "config.hpp"

namespace llcpp::detail::log_line {
    struct log_line_base {};
    
    template<typename Config, typename CharT, CharT... Chars>
    struct log_line : public log_line_base {
        using config_t = Config;
        using string_format = typename config_t::template string_format<config_t, CharT, Chars...>;

        template<CharT... Suffix>
        using log_line_with_suffix = log_line<config_t, CharT, Chars..., Suffix...>;
        template<CharT... Prefix>
        using log_line_with_prefix = log_line<config_t, CharT, Prefix..., Chars...>;
        template<typename _Config>
        using log_line_with_config = log_line<_Config, CharT, Chars...>;

        constexpr std::size_t size() const {
            return string_format::size();
        }
        static constexpr std::size_t fmt_size() {
            return string_format::fmt_size();
        }
        static constexpr std::size_t args_size() {
            return string_format::args_size();
        }

        template<typename Logger, typename... Args>
        void operator()(Logger& logger, Args&&... args) {
            using fmt_argument_tuple_size = std::tuple_size<typename string_format::format_parser::argument_tuple>;
            static_assert(sizeof...(Args) == fmt_argument_tuple_size::value,
                            "Discrepency between number of arguments in format and number of arguments in call");
            logger.write((std::uint8_t *)string_format::_chars, fmt_size());
            apply_args(logger, std::forward_as_tuple(args...));
        }

    private:
        using argument_tuple = typename string_format::format_parser::argument_tuple;
        using num_variable_args = format_parser::count_variable_arguments<argument_tuple>;

        template<typename Logger, typename ArgTuple, std::size_t Idx, typename Arg = typename std::tuple_element<Idx, ArgTuple>::type>
        void apply_arg(Logger& logger, const Arg arg) {
            using argument_parser = typename std::tuple_element_t<Idx, argument_tuple>;
            using _expected_arg_type_from_format = typename argument_parser::argument_type;
            //Add const to allow const arguments to be passed
            using expected_arg_type_from_format =
                typename std::conditional_t<
                    std::is_pointer_v<_expected_arg_type_from_format>,
                    //HACK: for char pointers
                    std::add_pointer_t<std::add_const_t<std::remove_pointer_t<_expected_arg_type_from_format>>>,
                    typename std::conditional_t<
                        std::is_array_v<_expected_arg_type_from_format>,
                        //HACK for char arrays
                        std::add_pointer_t<std::add_const_t<std::remove_all_extents_t<_expected_arg_type_from_format>>>,
                        std::add_const_t<_expected_arg_type_from_format>
                    >
                >;

            static_assert(std::is_convertible<Arg, expected_arg_type_from_format>::value,
                "Discrepency between argument type deduced from format and the given argument's type");

            argument_parser::apply(logger, arg);
        };

        template<typename Logger, typename ArgTuple, std::size_t Idx, typename Arg = typename std::tuple_element<Idx, ArgTuple>::type>
        void apply_arg_variable(Logger& logger, const Arg arg) {
            //No-Op for anything other than strings...
        }
        template<typename Logger, typename ArgTuple, std::size_t Idx, typename Arg = typename std::tuple_element<Idx, ArgTuple>::type>
        void apply_arg_variable(Logger& logger, const char * arg) {
            using argument_parser = typename std::tuple_element_t<Idx, argument_tuple>;

            if constexpr(!argument_parser::is_fixed_size) {
                argument_parser::apply_variable(logger, arg);
            }
        };
        template<typename Logger, typename ArgTuple, std::size_t... I>
        void _apply_args(Logger& logger, ArgTuple args, std::index_sequence<I...>) {
            (apply_arg<Logger, ArgTuple, I>(logger, std::get<I>(args)), ...);
            if constexpr(num_variable_args::value > 0) {
                (apply_arg_variable<Logger, ArgTuple, I>(logger, std::get<I>(args)), ...);
            }
        }
        template<typename Logger, typename... Args>
        void apply_args(Logger& logger, std::tuple<Args...> args) {
            _apply_args(logger, std::move(args), std::index_sequence_for<Args...>{});
        }
    };
}