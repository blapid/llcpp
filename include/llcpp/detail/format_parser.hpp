#pragma once

#include "terminators.hpp"
#include "utils.hpp"

namespace llcpp::detail::format_parser {

    template<bool IsEscaped, std::size_t EscapeIdx>
    struct format_parser_state {
        using is_escaped = std::integral_constant<bool, IsEscaped>;
        using escape_idx = std::integral_constant<std::size_t, EscapeIdx>;
    };

    template<typename Config, typename CharTuple, std::size_t CharTupleSize, std::size_t Idx, typename State>
    struct format_parser_impl {
        static_assert(utils::is_specialization_of<CharTuple, std::tuple>::value, "CharTuple must be a tuple");
        using config_t = Config;
        using cur_char = typename std::tuple_element<Idx, CharTuple>::type;
        using is_escape_char = typename std::conditional<
            cur_char::value == '%',
            std::true_type,
            std::false_type>::type;

        using escape_termination_t = typename terminators::terminator_container<Config>::template search_terminators_t<typename cur_char::value_type, cur_char::value>;
        using is_escape_termination = typename terminators::terminator_container<Config>::template is_terminator<typename cur_char::value_type, cur_char::value>;
        using is_next_state_escaped = typename std::conditional<
            (State::is_escaped::value && !is_escape_termination::value) ||
            (!State::is_escaped::value && is_escape_char::value),
            std::true_type,
            std::false_type>::type;
        using escape_idx = typename std::conditional<
            !State::is_escaped::value && is_escape_char::value,
            std::integral_constant<std::size_t, Idx>,
            typename State::escape_idx>::type;


        using current_argument_parser = typename utils::type_if_or<
            State::is_escaped::value,
            typename escape_termination_t::template argument_parser<CharTuple, escape_idx::value, Idx>,
            typename escape_termination_t::empty_parser
        >::type;

        using current_argument_tuple = typename std::conditional_t<
            (current_argument_parser::argument_size > 0),
            std::tuple<current_argument_parser>,
            typename std::tuple<>
        >;

        using arguments_tail =
            typename format_parser_impl<
                config_t,
                CharTuple,
                CharTupleSize,
                Idx + 1,
                format_parser_state<
                    is_next_state_escaped::value,
                    escape_idx::value
                >
            >::argument_tuple;
        using argument_tuple = utils::tuple_cat_t<current_argument_tuple, arguments_tail>;
    };
    template<typename Config, typename CharTuple, std::size_t CharTupleSize, typename State>
    struct format_parser_impl<Config, CharTuple, CharTupleSize, CharTupleSize, State> {
        using argument_tuple = std::tuple<>;
    };

    template<typename Config, typename CharT, CharT... Chars>
    struct format_parser {
        using config_t = Config;
        using format_size = std::integral_constant<std::size_t, sizeof...(Chars)>;
        using format_tuple = std::tuple<typename std::integral_constant<CharT, Chars>::type...>;
        using parser_impl = format_parser_impl<config_t, format_tuple, format_size::value, 0, format_parser_state<false, 0>>;

        using argument_tuple = typename parser_impl::argument_tuple;


        template<std::size_t Idx, std::size_t Size>
        struct sum_arguments_size_impl {
            static constexpr std::size_t value =
                std::tuple_element<Idx, argument_tuple>::type::argument_size +
                sum_arguments_size_impl<Idx + 1, Size>::value;
        };
        template<std::size_t Size>
        struct sum_arguments_size_impl<Size, Size> {
            static constexpr std::size_t value = 0;
        };

        struct sum_arguments_size {
            static constexpr std::size_t value = sum_arguments_size_impl<0, std::tuple_size<argument_tuple>::value>::value;
        };
    };

    template<typename ArgumentTuple, std::size_t Idx>
    struct accumulate_argument_parser_size {
        static_assert(Idx <= std::tuple_size_v<ArgumentTuple>, "Idx is out of bounds");
        using argument_parser = typename std::tuple_element_t<Idx - 1, ArgumentTuple>;
        static constexpr std::size_t value = argument_parser::argument_size +
            accumulate_argument_parser_size<ArgumentTuple, Idx - 1>::value;
    };
    template<typename ArgumentTuple>
    struct accumulate_argument_parser_size<ArgumentTuple, 0> {
        static constexpr std::size_t value = 0;
    };

    template<typename ArgumentTuple, std::size_t Idx>
    struct count_variable_arguments_impl {
        static_assert(Idx < std::tuple_size_v<ArgumentTuple>, "Idx is out of bounds");
        using argument_parser = typename std::tuple_element_t<Idx, ArgumentTuple>;
        static constexpr std::size_t value = (!argument_parser::is_fixed_size) ? (1) : (0) +
            count_variable_arguments_impl<ArgumentTuple, Idx - 1>::value;
    };
    template<typename ArgumentTuple>
    struct count_variable_arguments_impl<ArgumentTuple, 0> {
        using argument_parser = typename std::tuple_element_t<0, ArgumentTuple>;
        static constexpr std::size_t value = (!argument_parser::is_fixed_size) ? (1) : (0);
    };
    template<typename ArgumentTuple, std::size_t Idx = std::tuple_size_v<ArgumentTuple>>
    struct count_variable_arguments {
        static constexpr std::size_t value =
            count_variable_arguments_impl<ArgumentTuple, Idx - 1>::value;
    };
    template<>
    struct count_variable_arguments<std::tuple<>> {
        static constexpr std::size_t value = 0;
    };
    
}