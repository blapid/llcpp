# llcpp
Literal Logging for C++

## TL;DR
If the requirement of outputting a textual log from the C++ application is relaxed, we can create a faster logging framework. Enter `llcpp` (literal logging for c++). This framework uses C++17 template (and constexpr) meta-programming magic to reduce the number of allocations needed to zero and eliminates the need to format the data to human readable form. Later, a simple script can be used to transform the log into human readable form. Oh, and we get type safety for free.

Intrigued? skeptical? excited? angry? Read on!

## Status
This library is still in PoC mode. Feel free to try it and hack on it, but I can't promise a smooth ride just yet! 

Also, I'll appreciate any feedback you may have :)

## Contents

- [Features](#features)
- [Not textual?!](#not-textual)
- [Benchmarks](#benchmarks)
- [Example](#example)
- [Design Overview](#design-overview)
- [Extending](#extending)
- [Blog post](#blog-post)

## Features
- Fast: Low call site latency, low total CPU usage. See [benchmarks](#benchmarks)
- `printf`-like format syntax.
- Type safe: Mixing format type specifiers and arguments result in compilation error.
- No allocation for any log line.
- Header only.
- Available format specifiers: %d, %u, %x, %s. More coming soon.
- Prefixes available for line structure: Log level indication, Date/Time, Nanosecond timestamp.
- Synchronous: No independant state, when call returns the line has been written. May add async features in the future.
- Caveat: Output is *not* textual. See [this](#not-textual). Parser written in python is implemented and will output the textual log.
- Caveat: Requires C++17. Tested on Clang 5.0.1 (Ubuntu) and Clang 900 (MacOS).

## Not Textual?!
Yeah! Outputting textual logs is one of those "hidden" constrainsts/assumptions that no one talks about. If you think about it, you hardly ever read your logs directly. They're usually pulled and aggregated from your servers and go through a whole pipeline, adding a parser is no big deal. 

Once you free yourself from this constraint, it's possible to implement a logging framework thats even faster than the current state of the art. How much faster you ask?

## Benchmarks
Make sure you read [my post]() on benchmarking logging frameworks to get the whole story.

The following benchmark were run on Ubuntu. Compiled with Clang 5.0.1 with `--std=c++1z -flto -O3` flags. `/tmp` is mounted on an SSD. llcpp is presented with both timestamp prefix and date/time prefix (which is slower due to `localtime()`).

#### 1 Thread Logging To `/tmp` 

|Logger|     50th|     75th|     90th|     99th|   99.9th|    Worst|  Average |       Total Time (seconds)|
|:----:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|:---------------:|
|llcpp (timestamp) |  165|      171|      211|     2058|     6313|   149432|230.360544| 0.42 |
|llcpp (date/time) |  184|      195|      216|     2153|     3718|  1099523|260.166040| 0.53 |
|mini-async-logger |       286|      335|      361|     1348|     2884|   6204357|324.418935| 1.32 |
|NanoLog |  211|      248|      291|     1099|     3809| 2493347|798.895457| 3.01 |
|spdlog | 729|      751|      780|      973|     3277|    1057879|744.825963| 0.95 |
|g3log |  2300|     2819|     3230|     5603|    28176|   2249642|2490.365190| 3.86 |

#### 4 Threads Logging To `/tmp`

|Logger|     50th|     75th|     90th|     99th|   99.9th|    Worst|  Average |       Total Time (seconds)|
|:----:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|:---------------:|
| llcpp (timestamp) | 173|      191|      242|     7501|    36031|  2668978|458.101174| 1.61 |
|llcpp (date/time) | 220|      227|      270|    15515|    46522|  2162221|567.410498| 1.69 |
|mini-async-logger | 414|      581|      863|     2872|    24384|   5858754|612.069366| 5.52 |
|NanoLog |       338|      404|     1120|     1859|    24906| 1599487|845.320061| 12.81 |
|spdlog |      1100|     1193|     1238|     1373|    14022|  1312104|1032.874598| 1.91 |
|g3log |      1904|     2452|     3171|     9600|    37283|  6174296|2428.927793| 16.29 |

## Example
```c++
int main(int argc, char* argv[])
{
    using conf_t = llcpp::default_config
    using prefix_tuple_t = std::tuple<llcpp::log_level_prefix, llcpp::time_format_prefix<>>;
    using logger_t = llcpp::file_logger<prefix_tuple_t, conf_t>;
    auto logger = logger_t("./log.txt", {});
    
    logger.info("llcpp message #%d : This is some text for your pleasure. %s"_log, 0, "/dev/null");
    logger.info("llcpp message #%d : This is some text for your pleasure. %d %p"_log, 1, 42, 50);
    logger.info("llcpp message #%d : This is some text for your pleasure. %d %s %ld %llx. end."_log, 2, 42, "asdf", 24, 50);
        return 0;
}
```

## Binary Log Format
The binary format is pretty straightforward: write the null-terminated format string, then write the in-memory binary representation of each of the arguments. For arithmetic types (ints, floats, etc.), the size of each binary representation is derived from the format string (`%d` will be 4 bytes, `%lld` will be 8 bytes, etc.). For strings, we have two options: writing the length and then the string or passing information about the maximum length in the format string and then copying the minimum between the given maximum and the string's actual length.

Examples:
- `LOG("Hello world\n")` - would simply be written as the null terminated string, as you'd expect.
- `LOG("Hello world %d\n", 42)` - would write the null terminated string, followed by 4 bytes representing `42` (little endian).
- `LOG("Hello world %lld\n", 42)` - would write the null terminated string, followed by 8 bytes representing `42` (little endian).
- `LOG("Hello world %s\n", "42")` - would write the null terminated string, followed by 4 bytes representing `2` (the length of `"42"`) and 2 bytes with the actual string `"42"`
- `LOG("Hello world %8s\n", "42")` - would write the null terminated string, followed by 8 bytes, of which the first two would hold the string `"42"`. If the string given in the argument is bigger, it would be truncated.

## Design Overview
- User defined string literals are used to capture log format into variadic template parameters
-  `string_format` - Is specialized holds a static array of characters representing the format string
- `format_parser` - Parses the format string in compile time, generating a tuple of `argument_parser`'s which correspond to the deduced arguments from the format
- `terminators` - Are a collection of types that correspond to printf format type identifiers (such as `'d'`, `'s'`, `'u'`, etc.). These are used by the `format_parser` to deduce the types of the escape sequences in the format string. These also provide a specific `argument_parser` which will be aggregated by the `format_parser`.
- `log_line` - Uses the `string_format` and `format_parser` to serialize the arguments to the log file. Exposes a `operator()` function which takes variadic arguments which are validated with the `argument_parser`'s tuple provided by the `format_parser`.
- `logging` - Provide an interface for writing log parts to an output sink (file, network, etc.). Also handle `prefix`'es - a way to add structured data to your log lines (log level indication, timestamp, etc.).
- `config` - Allows the user to override internal classes such as `string_format`, `format_parser` and built-in `terminator` list for easy customization.

## Extending

### Adding loggers:
- Loggers *must* use the [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) and inherit from `logger_base`.
- Loggers *must* implement the following functions
    - `inline void line_hint_impl()` - Called by `logger_base` when an entire log line was written.
    - `inline void write_impl(const std::uint8_t *data, const std::size_t len)` - Called by the serialization code for each log line part.
- Loggers *should* let the user pass a PrefixTuple and Config by template parameters.

Example:

```c++
template<typename PrefixTuple, typename Config = config::default_config>
struct vector_logger : public logger_base<PrefixTuple, vector_logger<PrefixTuple>, Config> {

    vector_logger(std::vector<std::uint8_t>& vec, PrefixTuple&& prefix_tuple) : m_vec(vec),
        logger_base<PrefixTuple, vector_logger<PrefixTuple>>(std::forward<PrefixTuple>(prefix_tuple))
    {
    }

    inline void line_hint_impl() {
    }

    inline void write_impl(const std::uint8_t *data, const std::size_t len) {    
        m_vec.insert(m_vec.end(), data, data+len);
    }

private:
    std::vector<std::uint8_t>& m_vec;
};
```

### Adding terminators
- Terminators *must* inherit from `terminator<char, X>` where X is the character representing the type identifier in the format string (i.e. `'d'`, `'s'`, etc.). 
- Terminators *must* not interfere with other terminators. For example: Adding an `'l'` terminator would interfere with the `'l'` options of the `'d'` terminator (think about `'%lld'`).
- Terminators must declare `template <typename FormatTuple, std::size_t EscapeIdx, std::size_t TerminatorIdx>
    struct argument_parser` with the following static members and functions:
    - `static constexpr std::size_t argument_size` - The size used when serializing this argument
    - `using argument_type` - The C++ type that this `argument_parser` expect to receive from the caller when serializing.
    - `static constexpr bool is_fixed_size` - False if this `argument_parser` cannot know in compile-time the size that will be needed during serialization.
    - `template <typename Logger, typename Arg> inline static void apply(Logger& logger, const Arg arg)` - The serialization function. It *should* use `logger.write()` to write the serialized data.
    - `template <typename Logger> inline static void apply_variable(Logger& logger, const char *arg)` - An auxiliary function that will be called if `is_fixed_size` is false. It *should* be used to write auxiliary serialization data if necessary.
     
- Terminators *should* be declared in the `llcpp::user_terminators` namespace.

Example: 
```c++
namespace llcpp::user_terminators {
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
}
```

### Adding prefixes
- Prefixes *must* declare `template<typename Logger>
        inline void apply(logging::level::level_enum level, Logger& logger)`. This function should use the given logger to write the prefix as needed.

Example:
  
```c++
struct nanosec_time_prefix : public prefix_base {
    template<typename Logger>
    inline void apply(typename logging::level::level_enum level, Logger& logger) {
        auto now = std::chrono::system_clock::now();
        auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        "[%llu]: "_log(logger, now_ns);
    }
};
```

### Changing the configuration
You may extend the default configuration using the `config_with_XXX` declarations.
Example:
```c++
using conf_t = llcpp::default_config::config_with_additional_terminators<std::tuple<llcpp::user_terminators::p>>;

using prefix_tuple_t = std::tuple<llcpp::log_level_prefix, llcpp::time_format_prefix<>>;
using logger_t = llcpp::file_logger<prefix_tuple_t, conf_t>;
```

## Blog post
For more information and background, head over to my blog posts [here]()!