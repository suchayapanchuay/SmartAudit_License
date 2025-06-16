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
Copyright (C) Wallix 2018
Author(s): Jonathan Poelen
*/

#pragma once

#include "cxx/diagnostic.hpp"
#include "utils/sugar/array_view.hpp"
#include "utils/sugar/zstring_view.hpp"

namespace jln
{
    template<char... cs>
    struct string_c
    {
        static constexpr char const value[sizeof...(cs)+1]{cs..., '\0'};

        static constexpr char const* c_str() noexcept
        {
            return value;
        }

        static constexpr zstring_view zstring() noexcept
        {
            return zstring_view::from_null_terminated(value, sizeof...(cs));
        }

        static constexpr std::string_view sv() noexcept
        {
            return std::string_view(value, sizeof...(cs));
        }

        template<char... cs2>
        constexpr string_c<cs..., cs2...> operator+(string_c<cs2...> /*unused*/) const
        {
            return {};
        }
    };

    template<class, class>
    struct string_c_concat;

    template<char... xs, char... ys>
    struct string_c_concat<string_c<xs...>, string_c<ys...>>
    {
        using type = string_c<xs..., ys...>;
    };

    template<class x, class y>
    using string_c_concat_t = typename string_c_concat<x, y>::type;

    namespace literals
    {
        REDEMPTION_DIAGNOSTIC_PUSH()
        REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wgnu-string-literal-operator-template")
        REDEMPTION_DIAGNOSTIC_GCC_IGNORE("-Wpedantic")
        template<class C, C... cs>
        string_c<cs...> operator ""_c () noexcept
        {
            return {};
        }

        template<class C, C... cs>
        string_c<cs...> operator ""_s () noexcept
        {
            return {};
        }
        REDEMPTION_DIAGNOSTIC_POP()
    } // namespace literals

    template<unsigned long long x>
    struct ull_to_string_c;

    template<unsigned long long x>
    using ull_to_string_c_t = typename ull_to_string_c<x>::type;

    namespace detail
    {
        template<int n>
        struct ull_to_string_c_impl;

        template<>
        struct ull_to_string_c_impl<1>
        {
            template<unsigned long long x, char c1, char c2, char c3, char c4, char c5>
            using f = string_c<c5>;
        };

        template<>
        struct ull_to_string_c_impl<2>
        {
            template<unsigned long long x, char c1, char c2, char c3, char c4, char c5>
            using f = string_c<c4, c5>;
        };

        template<>
        struct ull_to_string_c_impl<3>
        {
            template<unsigned long long x, char c1, char c2, char c3, char c4, char c5>
            using f = string_c<c3, c4, c5>;
        };

        template<>
        struct ull_to_string_c_impl<4>
        {
            template<unsigned long long x, char c1, char c2, char c3, char c4, char c5>
            using f = string_c<c2, c3, c4, c5>;
        };

        template<>
        struct ull_to_string_c_impl<5>
        {
            template<unsigned long long x, char c1, char c2, char c3, char c4, char c5>
            using f = string_c<c1, c2, c3, c4, c5>;
        };

        template<>
        struct ull_to_string_c_impl<6>
        {
            template<unsigned long long x, char c1, char c2, char c3, char c4, char c5>
            using f = string_c_concat_t<
                ull_to_string_c_t<x/100000>,
                string_c<c1, c2, c3, c4, c5>
            >;
        };
    } // namespace detail

    template<unsigned long long x>
    struct ull_to_string_c
    {
        using type = typename detail::ull_to_string_c_impl<(
            x > 99999 ? 6 :
            x > 9999 ? 5 :
            x > 999 ? 4 :
            x > 99 ? 3 :
            x > 9 ? 2
                : 1
        )>::template f<
            x,
            char(x / 10000 % 10 + '0'),
            char(x / 1000 % 10 + '0'),
            char(x / 100 % 10 + '0'),
            char(x / 10 % 10 + '0'),
            char(x % 10 + '0')
        >;
    };

    namespace detail
    {
        template<bool>
        struct integer_to_string_c_t_impl;

        template<>
        struct integer_to_string_c_t_impl<true>
        {
            template<long long x>
            using f = string_c_concat_t<
                string_c<'-'>,
                ull_to_string_c_t<static_cast<unsigned long long>(-(x+1))+1u>
            >;
        };

        template<>
        struct integer_to_string_c_t_impl<false>
        {
            template<unsigned long long x>
            using f = ull_to_string_c_t<x>;
        };
    } // namespace detail

    template<class T, T x>
    using integer_to_string_c_t = typename detail::integer_to_string_c_t_impl<(x < 0)>
        ::template f<x>;
} // namespace jln
