/*
SPDX-FileCopyrightText: 2023 Wallix Proxies Team
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <ostream>
#include <string>
#include <algorithm>
#include <string_view>
#include <type_traits>
#include "utils/sugar/chars_to_int.hpp"

#ifndef NDEBUG
#include <cstdio>
#endif

namespace cli
{

enum class Res
{
    Ok,
    Exit,
    Help,
    // for "--"
    StopParsing,
    // for "-"
    NotOption,
    UnknownOption,
    MissingValue,
    // for --flag=... / -f=...
    NotValueWithValue,
    BadValueFormat,
};

struct ParseResult
{
    int opti;
    int argc;
    char const * const * argv;
    char const * str;
    Res res;
};

namespace parsers
{
    enum class ValueProperty
    {
        Optional,
        Required,
        NoValue,
    };

    using optional_value = std::integral_constant<ValueProperty, ValueProperty::Optional>;
    using required_value = std::integral_constant<ValueProperty, ValueProperty::Required>;
    using no_value = std::integral_constant<ValueProperty, ValueProperty::NoValue>;

    template<class T>
    constexpr bool is_optional_value
        = std::decay_t<T>::value_property::value == optional_value::value;

    template<class T>
    constexpr bool is_required_value
        = std::decay_t<T>::value_property::value == required_value::value;

    template<class T>
    constexpr bool is_no_value
        = std::decay_t<T>::value_property::value == no_value::value;

    template<class T>
    struct argname_value_traits
    {
        static constexpr std::string_view default_argname = "<value>";
    };

    template<class T>
    struct argname_parser_traits
    : argname_value_traits<void>
    {};
} // namespace parsers

namespace detail
{
    class uninit_t {};

    template<class T>
    constexpr bool is_uninit = std::is_same_v<std::decay_t<T>, uninit_t>;
}

template<
    class Short = detail::uninit_t,
    class Long = detail::uninit_t,
    class Parser = detail::uninit_t,
    class Argname = detail::uninit_t>
struct [[nodiscard]] Option
{
    Short _short_name {};
    Long _long_name {};
    Parser _parser {};
    std::string_view _help {};
    Argname _argname {};

    Option&
    help(std::string_view desc)
    {
        _help = desc;
        return *this;
    }

    Option<Short, Long, Parser, std::string_view>
    argname(std::string_view name)
    {
        if constexpr (!std::is_same_v<Parser, detail::uninit_t>) {
            static_assert(!parsers::is_no_value<Parser>,
                "argname() is used with a no_value parser");
        }

        return {
            _short_name,
            _long_name,
            _parser,
            _help,
            name,
        };
    }

    template<class P>
    auto
    parser(P parser)
    {
        // concept like
        static_assert(std::is_same_v<Res, decltype(parser(std::declval<ParseResult&>()))>);

        if constexpr (std::is_same_v<Argname, detail::uninit_t> && !parsers::is_no_value<P>) {
            return Option<Short, Long, P, std::string_view>{
                _short_name,
                _long_name,
                parser,
                _help,
                parsers::argname_parser_traits<P>::default_argname,
            };
        }
        else {
            static_assert(std::is_same_v<Argname, detail::uninit_t> || !parsers::is_no_value<P>,
                "argname() is used with a no_value parser");

            return Option<Short, Long, P, Argname>{
                _short_name,
                _long_name,
                parser,
                _help,
                _argname,
            };
        }
    }
};

inline Option<char, std::string_view>
option(char short_name, std::string_view long_name)
{
    return {short_name, long_name};
}

inline Option<detail::uninit_t, std::string_view>
option(std::string_view long_name)
{
    return {{}, long_name};
}

inline Option<char>
option(char short_name)
{
    return {short_name};
}


namespace detail
{
    template<class Ints, class... Opts>
    struct tuple_options;

    template<std::size_t, class Opt>
    struct indexed_option
    {
        Opt option;
    };

    template<std::size_t... Ints, class... Opts>
    struct tuple_options<std::index_sequence<Ints...>, Opts...>
    : indexed_option<Ints, Opts>...
    {
        template<class F>
        auto operator()(F&& f)
        {
            return f(static_cast<indexed_option<Ints, Opts>&>(*this).option...);
        }

        template<class F>
        auto operator()(F&& f) const
        {
            return f(static_cast<indexed_option<Ints, Opts> const&>(*this).option...);
        }
    };
} // namespace detail

template<class... Opts>
struct Options : detail::tuple_options<std::index_sequence_for<Opts...>, Opts...>
{};

struct Helper
{
    std::string_view s;
};

inline Helper helper(std::string_view s)
{
    return {s};
}

#ifndef NDEBUG
namespace detail
{
    struct ShortNameChecker
    {
        template<class... Opts>
        bool check(Opts const&... opts)
        {
            (..., init(opts));

            bool result = true;
            for (int& short_name : short_names) {
                if (short_name > 1) {
                    std::fprintf(
                        stderr, "duplicate short name %d times: %c\n",
                        short_name, char(&short_name - short_names));
                    std::fflush(stderr);
                    result = false;
                }
            }

            return result;
        }

    private:
        int short_names[256]{};

        static void init(Helper /*opt*/)
        {}

        template<class Opt>
        void init(Opt const& opt)
        {
            if constexpr (!std::is_same_v<uninit_t, decltype(opt._short_name)>) {
                ++short_names[static_cast<unsigned char>(opt._short_name)];
            }
        }
    };

    struct LongNameChecker
    {
        template<class... Opts>
        bool check(Opts const&... opts)
        {
            std::string_view long_names[sizeof...(opts)+1]{};
            last = long_names;
            (..., init(opts));

            auto* first = std::begin(long_names);
            std::sort(first, last);
            first = std::find_if(first, last, [](std::string_view s){
                return !s.empty();
            });
            return std::adjacent_find(first, last) == last;
        }

    private:
        std::string_view* last;

        static void init(Helper /*opt*/)
        {}

        template<class Opt>
        void init(Opt const& opt)
        {
            if constexpr (!std::is_same_v<uninit_t, decltype(opt._long_name)>) {
                *last++ = opt._long_name;
            }
        }
    };
} // namespace detail
#endif

template<class... Opts>
auto options(Opts&&... opts)
{
    assert(detail::ShortNameChecker().check(opts...) && "duplicate short name");
    assert(detail::LongNameChecker().check(opts...) && "duplicate long name");
    return Options<std::decay_t<Opts>...>{{
        {{static_cast<Opts&&>(opts)}}...
    }};
}


namespace detail
{
    constexpr bool parse_short_option(
        char const * /*s*/, ParseResult& /*pr*/, Helper const& /*helper*/, Res& /*res*/)
    {
        return true;
    }

    template<class Option>
    bool parse_short_option(char const * s, ParseResult& pr, Option const& opt, Res& res)
    {
        if constexpr (!std::is_same_v<uninit_t, decltype(opt._short_name)>) {
            if (*s != opt._short_name) {
                return true;
            }

            if constexpr (parsers::is_required_value<decltype(opt._parser)>) {
                int inc = 1;
                if (s[1]) {
                    pr.str = s + ((s[1] == '=') ? 2 : 1);
                }
                else if (pr.opti + 1 < pr.argc) {
                    pr.str = pr.argv[pr.opti+1];
                    ++inc;
                }
                else {
                    res = Res::MissingValue;
                    return false;
                }

                res = opt._parser(pr);
                if (res == Res::Ok) {
                    pr.opti += inc;
                }
            }
            else if constexpr (parsers::is_optional_value<decltype(opt._parser)>) {
                if (s[1] == '=') {
                    pr.str = s + 2;
                    res = opt._parser(pr);
                    if (res == Res::Ok) {
                        ++pr.opti;
                    }
                }
                else {
                    pr.str = nullptr;
                    res = opt._parser(pr);
                    if (res == Res::Ok && !s[1]) {
                        ++pr.opti;
                    }
                }
            }
            // is no_value
            else if (s[1] == '=') {
                res = Res::NotValueWithValue;
            }
            else {
                res = opt._parser(pr);
                if (res == Res::Ok && !s[1]) {
                    ++pr.opti;
                }
            }

            return false;
        }

        return true;
    }

    constexpr bool parse_long_option(
        std::string_view /*sv*/, ParseResult& /*pr*/, Helper const& /*helper*/, Res& /*res*/)
    {
        return true;
    }

    template<class Option>
    bool parse_long_option(std::string_view sv, ParseResult& pr, Option const& opt, Res& res)
    {
        if constexpr (!std::is_same_v<uninit_t, decltype(opt._long_name)>) {
            if (opt._long_name == sv) {
                char const* s = sv.end();
                if (!*s) {
                    int inc = 1;
                    if constexpr (parsers::is_required_value<decltype(opt._parser)>) {
                        if (pr.opti + 1 < pr.argc) {
                            ++inc;
                            pr.str = pr.argv[pr.opti+1];
                        }
                        else {
                            res = Res::MissingValue;
                            return false;
                        }
                    }
                    else {
                        pr.str = nullptr;
                    }

                    res = opt._parser(pr);
                    if (res == Res::Ok) {
                        pr.opti += inc;
                    }
                }
                else if constexpr (parsers::is_no_value<decltype(opt._parser)>) {
                    res = Res::NotValueWithValue;
                }
                else {
                    assert(*s == '=');
                    pr.str = s + 1;
                    res = opt._parser(pr);
                    if (res == Res::Ok) {
                        ++pr.opti;
                    }
                }

                return false;
            }
        }

        return true;
    }

    template<class... Opts>
    Res parse_short_options(char const * s, ParseResult& pr, Opts const&... opts)
    {
        Res r = Res::Ok;
        return (... && (r == Res::Ok && parse_short_option(s, pr, opts, r))) ? Res::UnknownOption : r;
    }

    template<class... Opts>
    Res parse_long_options(std::string_view s, ParseResult& pr, Opts const&... opts)
    {
        Res r = Res::Ok;
        return (... && (r == Res::Ok && parse_long_option(s, pr, opts, r))) ? Res::UnknownOption : r;
    }

    template<class... Opts>
    Res parse_options(ParseResult& pr, Opts const&... opts)
    {
        while (pr.opti < pr.argc) {
            auto * s = pr.argv[pr.opti];
            if (s[0] == '-' && s[1]) {
                Res res;
                if (s[1] == '-') {
                    if (s[2]) {
                        char const* begin = s+2;
                        char const* end = begin + 1;
                        while (*end && *end != '=') {
                            ++end;
                        }
                        res = parse_long_options(std::string_view(begin, end-begin), pr, opts...);
                    }
                    else {
                        return Res::StopParsing;
                    }
                }
                else {
                    auto const old_opti = pr.opti;
                    res = parse_short_options(++s, pr, opts...);
                    while (res == Res::Ok && old_opti == pr.opti) {
                        res = parse_short_options(++s, pr, opts...);
                    }
                }

                if (res != Res::Ok) {
                    return res;
                }
            }
            else {
                return Res::NotOption;
            }
        }
        return Res::Ok;
    }
} // namespace detail

template<class Tuple>
ParseResult parse(Tuple const& t, int const ac, char const * const av[])
{
    ParseResult pr;
    pr.argc = ac;
    pr.argv = av;
    pr.opti = 1;
    pr.str = nullptr;
    pr.res = t([&pr](auto const&... opts) {
#ifndef IN_IDE_PARSER
        return detail::parse_options(pr, opts...);
#else
        (void)pr;
        (void(opts), ...);
        return Res::Ok;
#endif
    });
    return pr;
}

template<class Tuple>
ParseResult parse(Tuple const& t, int const opti, int const ac, char const * const av[])
{
    ParseResult pr;
    pr.argc = ac;
    pr.argv = av;
    pr.opti = opti;
    pr.str = nullptr;
    pr.res = t([&pr](auto const&... opts) {
#ifndef IN_IDE_PARSER
        return detail::parse_options(pr, opts...);
#else
        (void)pr;
        (void(opts), ...);
        return Res::Ok;
#endif
    });
    return pr;
}

template<class... Opts>
ParseResult parse(int const ac, char const * const av[], Opts const&... opts)
{
    ParseResult pr;
    pr.argc = ac;
    pr.argv = av;
    pr.opti = 1;
    pr.str = nullptr;
#ifndef IN_IDE_PARSER
    pr.res = detail::parse_options(pr, opts...);
#else
    (void(opts), ...);
    pr.res = Res::Ok;
#endif
    return pr;
}


constexpr struct Ok_ : std::integral_constant<Res, Res::Ok> {} Ok {};

template<class T>
T operator, (T x, Ok_ /*unused*/)
{ return x; }


namespace detail
{
    template<class T>
    Res arg_parse_int(T& result, char const * s)
    {
        chars_to_int_result<T> r;
        if constexpr (std::is_unsigned_v<T>) {
            r = (*s == '0' && REDEMPTION_UNLIKELY(s[1] == 'x' || s[1] == 'X'))
                ? hexadecimal_chars_to_int<T>(s + 2)
                : decimal_chars_to_int<T>(s);
        }
        else {
            r = decimal_chars_to_int<T>(s);
        }

        if (r.ec == std::errc() && !*r.ptr) {
            result = r.val;
            return Res::Ok;
        }
        return Res::BadValueFormat;
    }
} // namespace detail

namespace arg_parsers
{
    inline Res arg_parse(short& result, char const * s)
    {
        return detail::arg_parse_int(result, s);
    }

    inline Res arg_parse(unsigned short& result, char const * s)
    {
        return detail::arg_parse_int(result, s);
    }

    inline Res arg_parse(int& result, char const * s)
    {
        return detail::arg_parse_int(result, s);
    }

    inline Res arg_parse(unsigned& result, char const * s)
    {
        return detail::arg_parse_int(result, s);
    }

    inline Res arg_parse(long& result, char const * s)
    {
        return detail::arg_parse_int(result, s);
    }

    inline Res arg_parse(unsigned long& result, char const * s)
    {
        return detail::arg_parse_int(result, s);
    }

    inline Res arg_parse(long long& result, char const * s)
    {
        return detail::arg_parse_int(result, s);
    }

    inline Res arg_parse(unsigned long long& result, char const * s)
    {
        return detail::arg_parse_int(result, s);
    }

    inline Res arg_parse(std::string& result, char const * s)
    {
        result = s;
        return Res::Ok;
    }

    inline Res arg_parse(std::string_view& result, char const * s)
    {
        result = s;
        return Res::Ok;
    }

    inline Res arg_parse(char const *& result, char const * s)
    {
        result = s;
        return Res::Ok;
    }

    template<class T, class = void>
    struct arg_parse_traits
    {
        static Res parse(T& result, char const* s)
        {
            return arg_parse(result, s);
        }
    };

    template<class T>
    struct arg_parse_traits<T, std::enable_if_t<std::is_enum_v<T>>>
    {
        static Res parse(T& result, char const* s)
        {
            std::underlying_type_t<T> tmp;
            auto r = detail::arg_parse_int(tmp, s);
            if (r == Res::Ok) {
                result = static_cast<T>(tmp);
            }
            return r;
        }
    };
} // namespace arg_parsers

namespace detail
{
    struct NoAction
    {
        template<class... T>
        Res operator()(T const&... /*unused*/) const
        {
            return Res::Ok;
        }
    };
} // namespace detail

namespace parsers
{
    template<class Act>
    struct trigger
    {
        using value_property = no_value;

        Res operator()(ParseResult& /*pr*/) const
        {
            const Res r = (this->act(), Ok);
            return r;
        }

        Act act;
    };

    template<class Act>
    struct required_arg
    {
        using value_property = required_value;

        Res operator()(ParseResult& pr) const
        {
            const Res r = (this->act(pr.str), Ok);
            return r;
        }

        Act act;
    };

    template<class Act>
    struct optional_arg
    {
        using value_property = optional_value;

        Res operator()(ParseResult& pr) const
        {
            const Res r = (this->act(pr.str), Ok);
            return r;
        }

        Act act;
    };

    template<class T, class Act = detail::NoAction>
    struct arg_location
    {
        using value_property = required_value;

        Res operator()(ParseResult& pr) const
        {
            Res r = arg_parsers::arg_parse_traits<T>::parse(*value, pr.str);
            if (r == Res::Ok) {
                r = (act(*value), Ok);
            }
            return r;
        }

        T* value;
        Act act {};
    };

    template<class T, class Act = detail::NoAction>
    struct value
    {
        using value_property = required_value;

        Res operator()(ParseResult& pr) const
        {
            if constexpr (std::is_constructible_v<T, char const*>) {
                return act(T(pr.str)), Ok;
            }
            else {
                T value;
                const Res r = arg_parsers::arg_parse_traits<T>::parse(value, pr.str);
                return (r == Res::Ok) ? (act(value), Ok) : r;
            }
        }

        Act act;
    };

    template<class T, class Act = detail::NoAction>
    struct password_location
    {
        using value_property = required_value;

        Res operator()(ParseResult& pr) const
        {
            const Res r = arg_location<T, Act const&>{value, act}(pr);
            if (r == Res::Ok) {
                char* s = argv[pr.opti] + (pr.str - pr.argv[pr.opti]);
                // hide password in /proc/...
                if (*s) {
                    *s++ = '*';
                    while (*s) {
                        *s++ = '\0';
                    }
                }
            }
            return r;
        }

        T* value;
        char** argv;
        Act act {};
    };

    template<class Act>
    struct on_off
    {
        using value_property = optional_value;

        Res operator()(ParseResult& pr) const
        {
            bool on = !pr.str;
            if (!on) {
                using namespace std::string_view_literals;
                std::string_view str = pr.str;
                on = str == "on"sv || str == "1"sv;
                if (!on && pr.str != "off"sv && pr.str != "0"sv) {
                    return Res::BadValueFormat;
                }
            }

            Res r = (this->act(on), Ok);
            return r;
        }

        Act act {};
    };

    struct on_off_location
    {
        using value_property = optional_value;

        Res operator()(ParseResult& pr) const
        {
            auto act = [&](bool state){ *value = state; };
            return on_off<decltype(act)>{act}(pr);
        }

        bool* value;
    };

    template<Res R, class Act = detail::NoAction>
    struct set_res
    {
        using value_property = no_value;

        Res operator()(ParseResult& /*pr*/) const
        {
            const Res r = (this->act(), Ok);
            return (r == Res::Ok) ? R : r;
        }

        Act act {};
    };

    template<class T, class Act>
    struct argname_parser_traits<password_location<T, Act>>
    {
        static constexpr std::string_view default_argname = "<password>";
    };

    template<>
    struct argname_parser_traits<on_off_location>
    {
        static constexpr std::string_view default_argname = "{on|off}";
    };

    template<class Act>
    struct argname_parser_traits<on_off<Act>>
    : argname_parser_traits<on_off_location>
    {};

    template<class T, class Act>
    struct argname_parser_traits<arg_location<T, Act>>
    : argname_value_traits<T>
    {};

    template<class T, class Act>
    struct argname_parser_traits<value<T, Act>>
    : argname_value_traits<T>
    {};
} // namespace parsers

template<class Act>
auto trigger(Act act)
{
    return parsers::trigger<Act>{act};
}

template<class T>
auto arg_increment(T& x)
{
    return trigger([&]{ ++x; });
}

template<class T, class Act = detail::NoAction>
auto arg_location(T& x, Act act = {})
{
    return parsers::arg_location<T, Act>{&x, act};
}

template<class T, class Act = detail::NoAction>
auto password_location(char** argv, T& x, Act act = {})
{
    return parsers::password_location<T, Act>{&x, argv, act};
}

template<class Act>
auto raw(Act act)
{
    return parsers::required_arg<Act>{act};
}

template<class Act>
auto on_off(Act act)
{
    return parsers::on_off<Act>{act};
}

template<class T>
auto on_off_location(T& x)
{
    return on_off([&](bool state){ x = state; });
}

inline auto on_off_location(bool& x)
{
    return parsers::on_off_location{&x};
}

inline auto arg_location(bool& x)
{
    return parsers::on_off_location{&x};
}

template<auto X, class T>
auto on_off_bit_location(T& value)
{
    return on_off([&](bool state){
        if (state) {
            value |= X;
        }
        else {
            value &= ~X;
        }
    });
}

template<class T, class Act>
auto value(Act act)
{

    return parsers::value<T, Act>{act};
}

template<class Act = detail::NoAction>
inline auto help(Act act = {})
{
    return parsers::set_res<Res::Help, Act>{act};
}

template<class Act = detail::NoAction>
inline auto quit(Act act = {})
{
    return parsers::set_res<Res::Exit, Act>{act};
}

namespace detail
{
    template<class Mem>
    struct first_mem_param;

    template<class R, class M, class T>
    struct first_mem_param<R(M::*)(T) const>
    {
        using type = std::decay_t<T>;
    };

    template<class R, class M>
    struct first_mem_param<R(M::*)() const>
    {
        using type = void;
    };
} // namespace detail

template<class Act>
auto arg(Act act)
{
    using T = typename detail::first_mem_param<decltype(&Act::operator())>::type;

    if constexpr (std::is_void_v<T>) {
        return parsers::trigger<Act>{act};
    }
    else {
        return parsers::value<T, Act>{act};
    }
}

namespace detail
{
    struct output_len_counter
    {
        int n = 0;

        output_len_counter& operator<<(std::string_view s)
        {
            n += int(s.size());
            return *this;
        }

        output_len_counter& operator<<(char /*c*/)
        {
            ++n;
            return *this;
        }
    };

    template<class OStream>
    OStream& print_option_name(OStream && out, Helper const& /*helper*/)
    {
        return out;
    }

    template<class OStream, class Opt>
    OStream& print_option_name(OStream && out, Opt const& opt)
    {
        constexpr bool has_short_name = !std::is_same_v<detail::uninit_t, decltype(opt._short_name)>;
        constexpr bool has_long_name = !std::is_same_v<detail::uninit_t, decltype(opt._long_name)>;

        if constexpr (has_short_name) {
            out << '-' << opt._short_name;
            if constexpr (has_long_name) {
                out << ", --" << opt._long_name;
            }
        }
        else {
            (void)has_long_name;
            out << "--" << opt._long_name;
        }

        if constexpr (parsers::is_required_value<decltype(opt._parser)>) {
            out << "=" << opt._argname;
        }
        else if constexpr (parsers::is_optional_value<decltype(opt._parser)>) {
            out << "[=" << opt._argname << "]";
        }

        return out;
    }
} // namespace detail

template<class Opt>
void print_help(std::ostream& out, Opt const& opt, int pad)
{
    out << "  ";
    detail::print_option_name(out, opt);

    // if constexpr (!std::is_same_v<detail::uninit_t, decltype(opt._help)>) {
    if (!opt._help.empty()) {
        out.width(pad);
        out << "" << opt._help;
    }
    out << "\n";
}

inline void print_help(std::ostream& out, Helper h, int /*pad*/)
{
    out << "\n" << h.s << "\n\n";
}

template<class Tuple>
void print_help(Tuple const& t, std::ostream& out, int maxlen = 32)
{
    t([&](auto... opts) {
        int const lengths[] = {0, detail::print_option_name(detail::output_len_counter{}, opts).n...};
        int const* p = lengths + 1;
        int max = std::min(maxlen, *std::max_element(p, std::end(lengths)));
        (..., print_help(out, opts, std::max(max + 4 - *p++, 2)));
    });
}

inline void print_error(ParseResult const& cli_result, std::ostream& out)
{
    if (Res::BadValueFormat == cli_result.res) {
        out << "Bad value at parameter ";
    }
    else if (Res::MissingValue == cli_result.res) {
        out << "Value is missing at parameter ";
    }
    else if (Res::UnknownOption == cli_result.res) {
        out << "Unknown option at parameter ";
    }
    else {
        out << "Bad option at parameter ";
    }

    out << cli_result.opti;

    if (cli_result.opti < cli_result.argc) {
        out << " (" << cli_result.argv[cli_result.opti] << ")";
    }
}

enum class CheckResult
{
    Error,
    Exit,
    Ok,
};

template<class Tuple>
CheckResult check_result(Tuple const& options, ParseResult const& cli_result, std::ostream& out, std::ostream& out_err)
{
    switch (cli_result.res) {
    case cli::Res::Ok:
        return CheckResult::Ok;

    case cli::Res::Exit:
        return CheckResult::Exit;

    case cli::Res::Help:
        // std::cout.sync_with_stdio(false);
        cli::print_help(options, out);
        return CheckResult::Exit;

    case cli::Res::BadValueFormat:
    case cli::Res::MissingValue:
    case cli::Res::UnknownOption:
    case cli::Res::NotValueWithValue:
    case cli::Res::NotOption:
    case cli::Res::StopParsing:
        break;
    }

    cli::print_error(cli_result, out_err);
    out_err << "\n";
    return CheckResult::Error;
}

} // namespace cli
