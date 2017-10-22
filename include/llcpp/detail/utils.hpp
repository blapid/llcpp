#pragma once

namespace llcpp::detail::utils {
    /*
     * A little static_assert debug utility.
     * The extra `Args...` are shown pretty close to the static_assert in the compilation
     *  log which allows us to give us hints on the issue.
     */
    template<bool Bool, typename... Args>
    struct StaticAssert {
        static constexpr bool value = Bool;
        static_assert(Bool, "DONT PANIC");
    };

    /*
     * A little hack to create a std::conditional like struct that won't evaluate
     *  both branches (which sometimes lead to errors on the other branch)
     */
    template<bool B, typename T, typename Else>
    struct type_if_or {
        using type = Else;
    };
    template<typename T, typename Else>
    struct type_if_or<true, T, Else> {
        using type = T;
    };
    
    //https://bitbucket.org/martinhofernandes/wheels/src/default/include/wheels/meta/type_traits.h%2B%2B?fileviewer=file-view-default#cl-161
    //Example: static_assert(utils::is_specialization_of<CharTuple, std::tuple>::value, "CharTuple must be a tuple");
    template <typename T, template <typename...> class Template>
    struct is_specialization_of : std::integral_constant<bool, false> {};
    template <template <typename...> class Template, typename... Args>
    struct is_specialization_of<Template<Args...>, Template> : std::integral_constant<bool, true> {};
    
    /*
     * Type of two tuples concatenated
     */
    template<typename TupleA, typename TupleB>
    using tuple_cat_t = decltype(std::tuple_cat<TupleA, TupleB>(TupleA(),TupleB()));
}