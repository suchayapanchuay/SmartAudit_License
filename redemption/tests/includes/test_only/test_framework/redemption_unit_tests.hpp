/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Product name: redemption, a FLOSS RDP proxy
Copyright (C) Wallix 2010-2018
Author(s): Jonathan Poelen
*/

#pragma once

#include "cxx/diagnostic.hpp"
#include "utils/sugar/bytes_view.hpp"
#include <iterator> // std:begin / std::end
#include <type_traits>


namespace ut
{
    enum class PatternView : char
    {
        deduced = 'a',
        ascii = 'c',
        ascii_nl = 'C',
        utf8 = 's',
        utf8_nl = 'S',
        hex = 'b',
        dump = 'd',
    };

    extern PatternView default_pattern_view; /*NOLINT*/
    extern unsigned default_ascii_min_len; /*NOLINT*/

    struct flagged_bytes_view
    {
        using value_type = uint8_t;

        bytes_view bytes;
        PatternView pattern = default_pattern_view;
        unsigned min_len = default_ascii_min_len;
    };

    inline flagged_bytes_view dump(bytes_view v) { return {v, PatternView::dump, 0}; }
    inline flagged_bytes_view hex(bytes_view v) { return {v, PatternView::hex, 0}; }
    inline flagged_bytes_view utf8(bytes_view v) { return {v, PatternView::utf8, 0}; }
    inline flagged_bytes_view ascii(bytes_view v, unsigned min_len = default_ascii_min_len)
    { return {v, PatternView::ascii, min_len}; }

    bool compare_bytes(size_t& pos, bytes_view b, bytes_view a) noexcept;
    void put_view(size_t pos, std::ostream& out, flagged_bytes_view x);

    void put_message_with_diff(std::ostream& out, ::chars_view s1, char const* op, ::chars_view s2, bool nocolor = false); /*NOLINT*/

    void print_hex(std::ostream& out, uint64_t x, int ndigit = 0);

    struct PatternViewSaver
    {
        PatternViewSaver(PatternView pattern) noexcept;
        ~PatternViewSaver();

    private:
        PatternView pattern;
    };

    struct AsciiMinLenSaver
    {
        AsciiMinLenSaver(unsigned min_len) noexcept;
        ~AsciiMinLenSaver();

    private:
        unsigned min_len;
    };

    struct hex_int
    {
        int ndigit = 0;

        constexpr static hex_int u8() { return {2}; }
        constexpr static hex_int u16() { return {4}; }
        constexpr static hex_int u32() { return {8}; }
        constexpr static hex_int u64() { return {8}; }
    };
} // namespace ut



namespace redemption_unit_test_
{
    namespace literals
    {
        inline ut::flagged_bytes_view operator""_av_ascii(char const * s, size_t len) noexcept
        {
            return ut::ascii({s, len});
        }

        inline ut::flagged_bytes_view operator""_av_utf8(char const * s, size_t len) noexcept
        {
            return ut::utf8({s, len});
        }

        inline ut::flagged_bytes_view operator""_av_hex(char const * s, size_t len) noexcept
        {
            return ut::hex({s, len});
        }

        inline ut::flagged_bytes_view operator""_av_dump(char const * s, size_t len) noexcept
        {
            return ut::dump({s, len});
        }
    } // namespace literals
} // namespace redemption_unit_test_


#if !defined(REDEMPTION_UNIT_TEST_FAST_CHECK)
# define REDEMPTION_UNIT_TEST_FAST_CHECK 0
#endif

#if defined(IN_IDE_PARSER)
# define REDEMPTION_UT_OSTREAM_PLACEHOLDER(out) ::redemption_unit_test_::Stream{}
# define REDEMPTION_UT_UNUSED_STREAM [[maybe_unused]]
#else
# define REDEMPTION_UT_OSTREAM_PLACEHOLDER(out) out
# define REDEMPTION_UT_UNUSED_STREAM
#endif

#if defined(IN_IDE_PARSER) && !defined(REDEMPTION_UNIT_TEST_CPP)

namespace redemption_unit_test_
{
    void X(bool);

    struct Stream
    {
        template<class T>
        Stream operator<<(T const&)
        {
            return *this;
        }

        explicit operator bool() const { return true; }
    };

    template<class> class red_test_print_type_t;

    template<class F>
    struct fn_caller
    {
        F f;

        template<class... Ts>
        constexpr auto operator()(Ts&&... xs) const
        {
            return f(static_cast<Ts>(xs)...);
        }
    };

    template<class F>
    constexpr fn_caller<F> fn_invoker(char const* /*name*/, F f);
} // namespace redemption_unit_test_

template<class T, class U>  bool operator==(array_view<T> const&, array_view<U> const&) { return true; }
template<class T> bool operator==(array_view<T> const&, bytes_view const&) { return true; }
template<class U> bool operator==(bytes_view const&, array_view<U> const&) { return true; }
bool operator==(bytes_view const&, bytes_view const&);
template<class T> bool operator==(array_view<T> const&, ut::flagged_bytes_view const&) { return true; }
template<class U> bool operator==(ut::flagged_bytes_view const&, array_view<U> const&) { return true; }
bool operator==(ut::flagged_bytes_view const&, bytes_view const&);
bool operator==(bytes_view const&, ut::flagged_bytes_view const&);

template<class T, class U>  bool operator!=(array_view<T> const&, array_view<U> const&) { return true; }
template<class T> bool operator!=(array_view<T> const&, bytes_view const&) { return true; }
template<class U> bool operator!=(bytes_view const&, array_view<U> const&) { return true; }
bool operator!=(bytes_view const&, bytes_view const&);
template<class T> bool operator!=(array_view<T> const&, ut::flagged_bytes_view const&) { return true; }
template<class U> bool operator!=(ut::flagged_bytes_view const&, array_view<U> const&) { return true; }
bool operator!=(ut::flagged_bytes_view const&, bytes_view const&);
bool operator!=(bytes_view const&, ut::flagged_bytes_view const&);

# define FIXTURES_PATH "./tests/fixtures"
# define CFG_PATH "./sys/etc/rdpproxy"

# define RED_FAIL(ostream_expr) ::redemption_unit_test_::Stream{} << ostream_expr
# define RED_ERROR(ostream_expr) ::redemption_unit_test_::Stream{} << ostream_expr
# define RED_TEST_INFO(ostream_expr) ::redemption_unit_test_::Stream{} << ostream_expr
# define RED_TEST_CHECKPOINT(ostream_expr) ::redemption_unit_test_::Stream{} << ostream_expr
# define RED_TEST_MESSAGE(ostream_expr) ::redemption_unit_test_::Stream{} << ostream_expr
# define RED_TEST_PASSPOINT() do { } while(0)
# define RED_TEST_DONT_PRINT_LOG_VALUE(type)
# define RED_TEST_PRINT_TYPE_FUNCTION_NAME red_test_print_type
# define RED_TEST_PRINT_TYPE_STRUCT_NAME redemption_unit_test_::red_test_print_type_t


# define RED_AUTO_TEST_CASE(test_name)       \
    struct test_name { void operator()(); }; \
    void test_name::operator()()

# define RED_FIXTURE_TEST_CASE(test_name, F)     \
    struct test_name : F { void operator()(); }; \
    void test_name::operator()()

/// CHECK
//@{
# define RED_TEST(expr) RED_TEST_CHECK(expr)
# define RED_TEST_CHECK(expr) RED_CHECK(expr)
# define RED_TEST_CONTEXT(ostream_expr) if (::redemption_unit_test_::Stream{} << ostream_expr)

# define RED_TEST_INVOKER(fname) fname
# define RED_TEST_FUNC_CTX(fname) ([](auto&&... xs) { \
    return fname(static_cast<decltype(xs)&&>(xs)...); })
# define RED_TEST_FUNC(fname, ...) ::redemption_unit_test_::X(bool(fname __VA_ARGS__))
# define RED_REQUIRE_FUNC(fname, ...) ::redemption_unit_test_::X(bool(fname __VA_ARGS__))

# define RED_CHECK_EXCEPTION_ERROR_ID(stmt, id) do { stmt; (void)id; } while (0)
# define RED_CHECK_NO_THROW(stmt) do { stmt; } while (0)
# define RED_CHECK_THROW(stmt, exception) do { stmt; [](exception) {}; } while (0)
# define RED_CHECK_EXCEPTION(stmt, exception, predicate) do { \
    stmt; [](exception & e) { predicate(e); }; } while (0)
# define RED_CHECK_EQUAL(a, b) ::redemption_unit_test_::X(bool((a) == (b)))
# define RED_CHECK_NE(a, b) ::redemption_unit_test_::X(bool((a) != (b)))
# define RED_CHECK_LT(a, b) ::redemption_unit_test_::X(bool((a) < (b)))
# define RED_CHECK_LE(a, b) ::redemption_unit_test_::X(bool((a) <= (b)))
# define RED_CHECK_GT(a, b) ::redemption_unit_test_::X(bool((a) > (b)))
# define RED_CHECK_GE(a, b) ::redemption_unit_test_::X(bool((a) >= (b)))
# define RED_CHECK(a) ::redemption_unit_test_::X(bool(a))
# define RED_CHECK_MESSAGE(a, ostream_expr) ::redemption_unit_test_::X(bool(a)), \
    ::redemption_unit_test_::Stream{} << ostream_expr
# define RED_CHECK_EQUAL_COLLECTIONS(first1, last1, first2, last2) \
    ::redemption_unit_test_::X(first1 == last1 && first2 == last2)
# define RED_CHECK_EQUAL_RANGES(a, b) ::redemption_unit_test_::X((void(a), void(b), true))
# define RED_CHECK_PREDICATE(pred, arg_list) pred arg_list
//@}

/// REQUIRE
//@{
# define RED_TEST_REQUIRE(expr) RED_REQUIRE(expr)
# define RED_REQUIRE_EXCEPTION_ERROR_ID(stmt, id) do { stmt; (void)id; } while (0)
# define RED_REQUIRE_NO_THROW(stmt) do { stmt; } while (0)
# define RED_REQUIRE_THROW(stmt, exception) do { stmt; [](exception) {}; } while (0)
# define RED_REQUIRE_EXCEPTION(stmt, exception, predicate) do { \
    stmt; [](exception & e) { predicate(e); }; } while (0)
# define RED_REQUIRE_EQUAL(a, b) ::redemption_unit_test_::X(bool((a) == (b)))
# define RED_REQUIRE_NE(a, b) ::redemption_unit_test_::X(bool((a) != (b)))
# define RED_REQUIRE_LT(a, b) ::redemption_unit_test_::X(bool((a) < (b)))
# define RED_REQUIRE_LE(a, b) ::redemption_unit_test_::X(bool((a) <= (b)))
# define RED_REQUIRE_GT(a, b) ::redemption_unit_test_::X(bool((a) > (b)))
# define RED_REQUIRE_GE(a, b) ::redemption_unit_test_::X(bool((a) >= (b)))
# define RED_REQUIRE(a) ::redemption_unit_test_::X(bool(a))
# define RED_REQUIRE_MESSAGE(a, ostream_expr) ::redemption_unit_test_::X(bool(a)), \
    ::redemption_unit_test_::Stream{} << ostream_expr
# define RED_REQUIRE_EQUAL_COLLECTIONS(first1, last1, first2, last2) \
    ::redemption_unit_test_::X(first1 == last1 && first2 == last2)
# define RED_REQUIRE_EQUAL_RANGES(a, b) ::redemption_unit_test_::X((void(a), void(b), true))
# define RED_REQUIRE_PREDICATE(pred, arg_list) pred arg_list
//@}

/// WARN
//@{
# define RED_TEST_WARN(expr) RED_CHECK(expr)
//@}

#define RED_TEST_CREATE_DECORATOR(name, f)

#else

# include "impl/redemption_unit_tests_impl.hpp"

/// CHECK
//@{
# define RED_CHECK_EXCEPTION_ERROR_ID(stmt, ErrId) \
    RED_TEST_EXCEPTION_ERROR_ID(CHECK, stmt, ErrId)

# define RED_CHECK_EQUAL_RANGES(a, b) \
    RED_TEST_EQUAL_RANGES(CHECK, a, b)
//@}

/// REQUIRE
//@{
# define RED_REQUIRE_EXCEPTION_ERROR_ID(stmt, ErrId) \
    RED_TEST_EXCEPTION_ERROR_ID(REQUIRE, stmt, ErrId)

# define RED_REQUIRE_EQUAL_RANGES(a, b) \
    RED_TEST_EQUAL_RANGES(REQUIRE, a, b)
//@}

# define RED_TEST_STRING_CHECK "check"
# define RED_TEST_STRING_REQUIRE "require"

# define RED_TEST_EXCEPTION_ERROR_ID(lvl, stmt, ErrId) \
    RED_##lvl##_EXCEPTION(                             \
        stmt, Error,                                   \
        [&](Error const & e_) {                        \
            RED_CHECK_EQUAL(e_.id, ErrId);             \
            return (e_.id == (ErrId));                 \
        }                                              \
    )

# define RED_TEST_EQUAL_RANGES(lvl, a, b)   \
    [](auto const & a_, auto const & b_) {  \
        using std::begin;                   \
        using std::end;                     \
        RED_##lvl##_EQUAL_COLLECTIONS(      \
            (void(#a), begin(a_)), end(a_), \
            (void(#b), begin(b_)), end(b_)  \
        );                                  \
    }(a, b)

namespace redemption_unit_test_
{
    template<class E, E value>
    struct EnumValue
    {
        static ::chars_view str()
        {
            return cstr_array_view(__PRETTY_FUNCTION__);
        }
    };

    struct Enum
    {
        template<class E, class = std::enable_if_t<std::is_enum<E>::value>>
        Enum(E e) noexcept
        : Enum(
            static_cast<long long>(e),
            cstr_array_view(__PRETTY_FUNCTION__),
            std::is_signed_v<std::underlying_type_t<E>>,
            EnumValue<E, E(0)>::str(),
            EnumValue<E, E(1)>::str(),
            EnumValue<E, E(2)>::str(),
            EnumValue<E, E(3)>::str(),
            EnumValue<E, E(4)>::str(),
            EnumValue<E, E(5)>::str(),
            EnumValue<E, E(6)>::str(),
            EnumValue<E, E(7)>::str(),
            EnumValue<E, E(8)>::str(),
            EnumValue<E, E(9)>::str()
        )
        {}

        ::chars_view name;
        ::chars_view value_name;
        long long x;
        bool is_signed;

    private:
        Enum(
            long long x, ::chars_view proto, bool is_signed,
            ::chars_view s0, ::chars_view s1, ::chars_view s2,
            ::chars_view s3, ::chars_view s4, ::chars_view s5,
            ::chars_view s6, ::chars_view s7, ::chars_view s8,
            ::chars_view s9
        ) noexcept;
    };

    struct BytesView
    {
        BytesView(bytes_view bytes) noexcept : bytes(bytes) {}
        BytesView(writable_bytes_view bytes) noexcept : bytes(bytes) {}
        BytesView(chars_view bytes) noexcept : bytes(bytes) {}
        BytesView(u8_array_view bytes) noexcept : bytes(bytes) {}
        // BytesView(array_view_const_s8 bytes) noexcept : bytes(bytes) {}

        bytes_view bytes;
    };

} // namespace redemption_unit_test_

#if ! REDEMPTION_UNIT_TEST_FAST_CHECK

namespace std /*NOLINT*/
{
    // hack hack hack :D
    std::ostream& operator<<(std::ostream& out, ::redemption_unit_test_::Enum const& e);
    std::ostream& operator<<(std::ostream& out, ::redemption_unit_test_::BytesView const& v);
}

#endif

#endif

namespace redemption_unit_test_
{
    #if REDEMPTION_UNIT_TEST_FAST_CHECK || defined(IN_IDE_PARSER)
    template<class F>
    struct DatasChain
    {
        F f;

        template<class... Xs>
        DatasChain operator()(Xs&&... xs)
        {
            f(xs...);
            return *this;
        }
    };
    #else
    template<class F>
    struct OStreamFunc
    {
        F f;
    };

    template<class FF>
    std::ostream& operator<<(std::ostream& out, OStreamFunc<FF> const& of)
    {
        of.f(out);
        return out;
    }

    template<int N>
    struct DatasFuncCounter
    {
        template<class... Xs>
        DatasFuncCounter<N+1> operator()(Xs const&... /*xs*/)
        {
            return {};
        }

        static const int count = N;
    };

    template<class F>
    struct DatasChain
    {
        int i;
        int n;
        F f;

        template<class... Xs>
        DatasChain operator()(Xs&&... xs)
        {
            auto print = [&](std::ostream& out){
                out << "[" << i << "/" << n << "] (";
                (..., (out << xs << ", "));
                out << ")";
            };
            ++i;
            RED_TEST_CONTEXT(OStreamFunc<decltype(print)>{print}) {
                f(xs...);
            }

            return *this;
        }
    };
    #endif

    template<class FD>
    struct DatasCtx
    {
        FD fd;

        template<class F>
        void operator>>=(F f)
        {
            #if REDEMPTION_UNIT_TEST_FAST_CHECK || defined(IN_IDE_PARSER)
            fd(DatasChain<F>{f});
            #else
            fd(DatasChain<F>{0, decltype(fd(DatasFuncCounter<0>()))::count, f});
            #endif
        }
    };

    template<class FD>
    DatasCtx<FD> make_datas_ctx(FD fd)
    {
        return {fd};
    }
} // namespace redemption_unit_test_

#define RED_TEST_DATAS(datas) ::redemption_unit_test_ \
    ::make_datas_ctx([](auto f) { return f datas; })

# define RED_TEST_UNUSED_IDENT2_II(a, b) \
    [[maybe_unused]] inline const int a##b = 0
# define RED_TEST_UNUSED_IDENT2_I(a, b) \
    RED_TEST_UNUSED_IDENT2_II(a, b)

#if REDEMPTION_UNIT_TEST_FAST_CHECK
# define RED_TEST_DELEGATE_PRINT(type, stream_expr) \
    RED_TEST_UNUSED_IDENT2_I(TU_delegate_print_unused_, __LINE__)
# define RED_TEST_DELEGATE_OSTREAM(type, stream_expr) \
    RED_TEST_UNUSED_IDENT2_I(TU_delegate_stream_unused_, __LINE__)
#else
#define RED_TEST_DELEGATE_PRINT(type, stream_expr)         \
    template<>                                             \
    struct RED_TEST_PRINT_TYPE_STRUCT_NAME<type>           \
    {                                                      \
        void operator()(                                   \
            REDEMPTION_UT_UNUSED_STREAM std::ostream& out, \
            type const& _                                  \
        ) const                                            \
        {                                                  \
            REDEMPTION_UT_OSTREAM_PLACEHOLDER(out)         \
            << stream_expr; /* NOLINT */                   \
        }                                                  \
    }
# define RED_TEST_DELEGATE_OSTREAM_II(a, b) \
    [[maybe_unused]] inline int a##b = 0
# define RED_TEST_DELEGATE_OSTREAM_I(type, stream_expr) \
    RED_TEST_DELEGATE_OSTREAM_II(a, b)
# define RED_TEST_DELEGATE_OSTREAM(type, stream_expr)  \
    inline std::ostream& operator<<(                   \
        REDEMPTION_UT_UNUSED_STREAM std::ostream& out, \
        type const& _                                  \
    )                                                  \
    {                                                  \
        REDEMPTION_UT_OSTREAM_PLACEHOLDER(out)         \
        << stream_expr; /* NOLINT */                   \
        return out;                                    \
    }                                                  \
    /* for ; */                                        \
    RED_TEST_UNUSED_IDENT2_I(TU_delegate_stream_unused_, __LINE__)
#endif

#define RED_TEST_DELEGATE_PRINT_ENUM(type) \
    RED_TEST_DELEGATE_PRINT(type,          \
        #type << "{" << +::std::underlying_type_t<type>(_) << "}")


#if defined(IN_IDE_PARSER) || REDEMPTION_UNIT_TEST_FAST_CHECK
#define RED_TEST_CONTEXT_DATA(type_value, iocontext, ...) \
    for ([[maybe_unused]] type_value : __VA_ARGS__)       \
        RED_TEST_CONTEXT(iocontext) /*NOLINT*/
#else
#define RED_TEST_CONTEXT_DATA_II(cont, i, n, type_value, iocontext, ...) \
    if (auto&& cont = __VA_ARGS__; 1)                                    \
        if (::std::size_t i = 0,                                         \
            n = ::redemption_unit_test_::cont_size(cont, 1); 1          \
        )                                                                \
            for (type_value : cont)                                      \
                if (++i)                                                 \
                    RED_TEST_CONTEXT(                                    \
                        "[" << i << "/" << n << "] " << iocontext) /*NOLINT*/
#define RED_TEST_CONTEXT_DATA_I(cont, i, n, type_value, iocontext, ...) \
    RED_TEST_CONTEXT_DATA_II(cont, i, n, type_value, iocontext, __VA_ARGS__)

#define RED_TEST_CONTEXT_DATA(type_value, iocontext, ...) \
    RED_TEST_CONTEXT_DATA_I(                              \
        BOOST_PP_CAT(ctx_cont_, __LINE__),                \
        BOOST_PP_CAT(ctx_cont_i_, __LINE__),              \
        BOOST_PP_CAT(ctx_cont_n_, __LINE__),              \
        type_value, iocontext, __VA_ARGS__)
#endif

#define RED_AUTO_TEST_CONTEXT_DATA(test_name, type_value, iocontext, ...) \
    struct test_name##_case : ::redemption_unit_test_::MaxError           \
    { void _exec(type_value); };                                          \
    RED_FIXTURE_TEST_CASE(test_name, test_name##_case)                    \
    {                                                                     \
        auto l = __VA_ARGS__;                                             \
        auto p = l.begin();                                               \
        RED_TEST_CONTEXT_DATA(type_value, iocontext, l)                   \
        {                                                                 \
            auto const err_count = RED_ERROR_COUNT();                     \
            _exec(*p);                                                    \
            bool const stop_when_error = stop_after_error(false);         \
            if (stop_when_error && err_count != RED_ERROR_COUNT()) {      \
                break;                                                    \
            }                                                             \
            ++p;                                                          \
        }                                                                 \
    }                                                                     \
    void test_name##_case::_exec(type_value)

namespace redemption_unit_test_
{

struct MaxError
{
    bool stop_after_error(bool value = true)
    {
        auto old = m_stop_after_error;
        m_stop_after_error = value;
        return old;
    }

private:
    bool m_stop_after_error = false;
};

template<class T>
auto cont_size(T const& cont, int /*one*/) -> decltype(std::size_t(cont.size()))
{
    return cont.size();
}

template<class T>
std::size_t cont_size(T const& cont, char /*one*/)
{
    using std::begin;
    using std::end;
    return end(cont) - cont(begin);
}

unsigned long current_count_error();

struct int_variation
{
    int left;
    int right;
    int value;
    int variant;
    bool is_percent;
};

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator==(T const& x, int_variation const& variation) noexcept
{
    return variation.left <= x && x <= variation.right;
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator!=(T const& x, int_variation const& variation) noexcept
{
    return !(x == variation);
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator<(T const& x, int_variation const& variation) noexcept
{
    return x < variation.right;
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator>(T const& x, int_variation const& variation) noexcept
{
    return x > variation.left;
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator<=(T const& x, int_variation const& variation) noexcept
{
    return x <= variation.right;
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator>=(T const& x, int_variation const& variation) noexcept
{
    return x >= variation.left;
}


template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator==(int_variation const& variation, T const& x) noexcept
{
    return x == variation;
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator!=(int_variation const& variation, T const& x) noexcept
{
    return !(x == variation);
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator<(int_variation const& variation, T const& x) noexcept
{
    return x >= variation;
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator>(int_variation const& variation, T const& x) noexcept
{
    return x <= variation;
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator<=(int_variation const& variation, T const& x) noexcept
{
    return x > variation;
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, bool>
operator>=(int_variation const& variation, T const& x) noexcept
{
    return x < variation;
}

struct def_variation1
{
    int variantion;
    bool is_percent;
};

struct def_variation2
{
    int variantion;
    bool is_percent;
};

inline def_variation2 operator-(def_variation1 const& variation) noexcept
{
    return {variation.variantion, variation.is_percent};
}

template<class T>
int_variation operator+(T const& x_, def_variation2 const& variation) noexcept
{
    const int x = x_;
    if (variation.is_percent) {
        auto a = x * variation.variantion / 100;
        return {x - a, x + a, x, variation.variantion, true};
    }
    return {x - variation.variantion, x + variation.variantion, x, variation.variantion, false};
}

namespace literals
{
    inline def_variation1 operator""_percent(unsigned long long x) noexcept { return {int(x), true}; }
    inline def_variation1 operator""_v(unsigned long long x) noexcept { return {int(x), false}; }
}

} // namespace redemption_unit_test_

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wheader-hygiene")
using namespace redemption_unit_test_::literals; // NOLINT
REDEMPTION_DIAGNOSTIC_POP()

template<>
struct RED_TEST_PRINT_TYPE_STRUCT_NAME<redemption_unit_test_::int_variation>
{
    void operator()(std::ostream& out, redemption_unit_test_::int_variation const & x) const;
};

namespace redemption_unit_test_
{
    template<class T>
    auto to_av(T&& x)
    {
        return make_array_view(x);
    }
}

/// CHECK
//@{
#define RED_CHECK_EQ RED_CHECK_EQUAL
#define RED_CHECK_EQ_RANGES RED_CHECK_EQUAL_RANGES
//@}

/// REQUIRE
//@{
#define RED_REQUIRE_EQ RED_REQUIRE_EQUAL
#define RED_REQUIRE_EQ_RANGES RED_REQUIRE_EQUAL_RANGES
//@}

#define RED_ERROR_COUNT() redemption_unit_test_::current_count_error()
