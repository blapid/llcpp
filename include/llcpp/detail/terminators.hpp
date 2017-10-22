#pragma once

#include "utils.hpp"

namespace llcpp::detail::terminators
{
    template <typename CharT, CharT Char, typename ArgumentType = void>
    struct terminator
    {
        using char_type = CharT;
        static constexpr CharT terminator_value = Char;

        template <typename FormatTuple, std::size_t EscapeIdx, std::size_t TerminatorIdx>
        struct argument_parser
        {
            static constexpr std::size_t argument_size = 0;
            using argument_type = ArgumentType;
            static constexpr bool is_fixed_size = true;

            template <typename Logger, typename Arg>
            inline static void apply(Logger& logger, const Arg arg) {}
            template <typename Logger>
            inline static void apply_variable(Logger& logger, const char *arg) {}
        };

        using empty_parser = argument_parser<typename std::tuple<>, 0, 0>;
    };

    struct void_terminator : public terminator<char, 0>
    {
    };

    struct pct_terminator : public terminator<char, '%'>
    {
        template <typename FormatTuple, std::size_t EscapeIdx, std::size_t TerminatorIdx>
        struct argument_parser : public terminator<char, '%'>::argument_parser<FormatTuple, EscapeIdx, TerminatorIdx>
        {
            static_assert(EscapeIdx == TerminatorIdx - 1, "Invalid %% escape. Either the format is wrong "
                                                        "or you are using a terminator we don't support");
        };
    };

    template <typename FormatTuple, typename CharT, CharT Char, std::size_t Idx, std::size_t EndIdx>
    struct tuple_counter
    {
        using cur_char = typename std::tuple_element<Idx, FormatTuple>::type;
        static constexpr std::size_t count = ((cur_char::value == Char) ? (1) : (0)) +
                                            tuple_counter<FormatTuple, CharT, Char, Idx + 1, EndIdx>::count;
    };
    template <typename FormatTuple, typename CharT, CharT Char, std::size_t EndIdx>
    struct tuple_counter<FormatTuple, CharT, Char, EndIdx, EndIdx>
    {
        static constexpr std::size_t count = 0;
    };

    struct d : public terminator<char, 'd'>
    {
        template <typename FormatTuple, std::size_t EscapeIdx, std::size_t TerminatorIdx>
        struct argument_parser
        {
            static constexpr bool is_fixed_size = true;
            static constexpr std::size_t num_of_l = tuple_counter<FormatTuple, char, 'l', EscapeIdx, TerminatorIdx>::count;
            static_assert(num_of_l < 3, "Invalid conversion specifier");
            static constexpr std::size_t argument_size = ((num_of_l == 0) ? (sizeof(std::uint32_t)) : (sizeof(std::uint32_t) * num_of_l));
            using argument_type = typename std::conditional<
                num_of_l == 0,
                int,
                typename std::conditional<
                    num_of_l == 1,
                    long,
                    long long>::type>::type;

            template <typename Logger, typename Arg>
            inline static void apply(Logger& logger, const Arg arg)
            {
                static_assert(std::is_integral<Arg>::value,
                            "Integral argument's apply function called with non-integral value");
                static_assert(argument_size >= sizeof(Arg),
                            "Discrepency detected between parsed argument_size and size of given arg, "
                            "you may need a specialized argument_parser");
                logger.write((std::uint8_t *)&arg, argument_size);
            }

            template <typename Logger>
            inline static void apply_variable(Logger& logger, const char *arg) {}
        };
    };
    struct u : public terminator<char, 'u'>
    {
        template <typename FormatTuple, std::size_t EscapeIdx, std::size_t TerminatorIdx>
        struct argument_parser : public d::argument_parser<FormatTuple, EscapeIdx, TerminatorIdx>
        {
            using base_argument_type = typename d::argument_parser<FormatTuple, EscapeIdx, TerminatorIdx>::argument_type;
            using argument_type = typename std::make_unsigned_t<base_argument_type>;
        };
        
    };
    struct x : public terminator<char, 'x'>
    {
        template <typename FormatTuple, std::size_t EscapeIdx, std::size_t TerminatorIdx>
        using argument_parser = d::argument_parser<FormatTuple, EscapeIdx, TerminatorIdx>;
    };

    //HACK: There's probably a better way to do this
    constexpr std::size_t pow(std::size_t base, std::size_t exp)
    {
        std::size_t tmp = 1;
        for (std::size_t i = 0; i < exp; i++)
        {
            tmp *= base;
        }
        return tmp;
    }

    //HACK: Should probably switch this to a constexpr function, just need to convert the tuple to a char array
    template <typename FormatTuple, std::size_t Idx, std::size_t EndIdx>
    struct tuple_atoi
    {
        using cur_char = typename std::tuple_element<Idx, FormatTuple>::type;
        static_assert(cur_char::value >= '0' && cur_char::value <= '9', "tuple_atoi encountered a non-digit character");
        static constexpr std::size_t exp = EndIdx - Idx;
        static constexpr std::size_t value = pow(10, exp - 1) * (cur_char::value - '0') + tuple_atoi<FormatTuple, Idx + 1, EndIdx>::value;
    };
    template <typename FormatTuple, std::size_t EndIdx>
    struct tuple_atoi<FormatTuple, EndIdx, EndIdx>
    {
        static constexpr std::size_t value = 0;
    };

    struct s : public terminator<char, 's'>
    {
        using variable_string_length_type = std::uint32_t;

        template <typename FormatTuple, std::size_t EscapeIdx, std::size_t TerminatorIdx>
        struct argument_parser
        {
            static constexpr std::size_t argument_size = ((EscapeIdx + 1 == TerminatorIdx) ? (sizeof(variable_string_length_type)) : (tuple_atoi<FormatTuple, EscapeIdx + 1, TerminatorIdx>::value));
            static constexpr bool is_fixed_size = (EscapeIdx + 1 != TerminatorIdx);

            using argument_type = typename std::conditional<
                is_fixed_size,
                char[argument_size],
                char *>::type;

            template <typename Logger>
            inline static void apply(Logger& logger, const char *arg)
            {
                if
                    constexpr(is_fixed_size)
                    {
                        auto len = strlen(arg);
                        if (len < argument_size)
                        {
                            logger.write((std::uint8_t *)arg, len);
                            // fill the rest with zeroes
                            static std::uint8_t zeroes[1024] = {0};
                            std::size_t diff = argument_size - len;
                            std::size_t zeroes_written = 0;
                            while (zeroes_written < diff)
                            {
                                std::size_t zeroes_to_write = std::min(diff - zeroes_written, sizeof(zeroes));
                                logger.write(zeroes, zeroes_to_write);
                                zeroes_written += zeroes_to_write;
                            }
                        }
                        else if (len >= argument_size)
                        {
                            logger.write((std::uint8_t *)arg, argument_size);
                        }
                    }
                else
                {
                    variable_string_length_type size = static_cast<variable_string_length_type>(std::strlen(arg));
                    logger.write((std::uint8_t *)&size, sizeof(size));
                }
            }

            template <typename Logger>
            inline static void apply_variable(Logger& logger, const char *arg)
            {
                if
                    constexpr(!is_fixed_size)
                    {
                        logger.write((std::uint8_t *)arg, std::strlen(arg));
                    }
            }
        };
    };

    template <typename TerminatorTuple, typename CharT, CharT Char, std::size_t Idx, std::size_t TerminatorTupleSize>
    struct search_terminators_impl
    {
        static_assert(std::is_same<typename std::tuple_element<Idx, TerminatorTuple>::type::char_type, CharT>::value,
                    "Discrepency between the format's element type and terminator element type");
        using type = typename std::conditional<
            std::tuple_element<Idx, TerminatorTuple>::type::terminator_value == Char,
            typename std::tuple_element<Idx, TerminatorTuple>::type,
            typename search_terminators_impl<TerminatorTuple, CharT, Char, Idx + 1, TerminatorTupleSize>::type>::type;
    };
    template <typename TerminatorTuple, typename CharT, CharT Char, std::size_t TerminatorTupleSize>
    struct search_terminators_impl<TerminatorTuple, CharT, Char, TerminatorTupleSize, TerminatorTupleSize>
    {
        using type = void_terminator;
    };

    template <typename TerminatorTuple, typename CharT, CharT Char>
    struct search_terminators
    {
        using type = typename search_terminators_impl<TerminatorTuple, CharT, Char, 0, std::tuple_size_v<TerminatorTuple>>::type;
    };

    template <typename TerminatorTuple>
    struct terminator_container_impl
    {
        static_assert(utils::is_specialization_of<TerminatorTuple, std::tuple>::value, "TerminatorTuple must be a tuple");

        template <typename CharT, CharT Char>
        using is_terminator = typename std::conditional<
            std::is_same<typename search_terminators<TerminatorTuple, CharT, Char>::type, void_terminator>::value,
            std::false_type,
            std::true_type>::type;

        template <typename CharT, CharT Char>
        using search_terminators_t = typename search_terminators<TerminatorTuple, CharT, Char>::type;
    };

    using builtin_terminator_tuple = std::tuple<d, u, x, s, pct_terminator>;

    template<typename Config>
    using terminator_tuple_from_config = utils::tuple_cat_t<typename Config::terminator_tuple, typename Config::additional_terminators>;
    template<typename Config>
    struct terminator_container : public terminator_container_impl<terminator_tuple_from_config<Config>>{};
}

namespace llcpp::user_terminators {
    using namespace llcpp::detail::terminators;
}