/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Product name: redemption, a FLOSS RDP proxy
*   Copyright (C) Wallix 2010-2016
*   Author(s): Jonathan Poelen
*/

#pragma once

#include <type_traits>
#include <limits>
#include <cassert>


// analogous to static_cast<> for integral types
// with an assert macro if the conversion is overflow or underflow.
template <class Dst, class Src>
constexpr Dst checked_cast(Src value) noexcept;


// analogous to static_cast<> for integral types,
// except that use std::clamp if the conversion is overflow or underflow.
template <class Dst, class Src>
constexpr Dst saturated_cast(Src value) noexcept;


// analogous to static_cast<> for integral types
// with an static_assert if the conversion is possibly overflow or underflow.
template <class Dst, class Src>
constexpr Dst safe_cast(Src value) noexcept;


namespace detail
{
    template<class T>
    using is_numeric_convertible = std::integral_constant<bool,
        std::is_integral<T>::value || std::is_enum<T>::value>;
}


// integer type with checked_cast
template<class T>
struct checked_int
{
    using value_type = T;

    constexpr checked_int(checked_int&&) noexcept = default;
    constexpr checked_int(checked_int const&) noexcept = default;

    constexpr checked_int& operator=(checked_int&&) noexcept = default;
    constexpr checked_int& operator=(checked_int const&) noexcept = default;

    template<class U>
    constexpr checked_int(U i) noexcept
    : i(checked_cast<T>(i))
    {}

    constexpr checked_int(T i) noexcept
    : i(i)
    {}

    template<class U>
    constexpr checked_int& operator=(U i) noexcept
    {
        this->i = checked_cast<T>(i);
        return *this;
    }

    constexpr checked_int& operator=(T i) noexcept
    {
        this->i = i;
        return *this;
    }

    template<class U, class = std::enable_if_t<detail::is_numeric_convertible<U>::value>>
    constexpr operator U () const noexcept { return checked_cast<U>(i); }

    constexpr operator T () const noexcept { return this->i; }

    constexpr T underlying() const noexcept { return this->i; }

private:
    T i;
};


// integer type with saturated_cast
template<class T>
struct saturated_int
{
    using value_type = T;

    constexpr saturated_int(saturated_int&&) noexcept = default;
    constexpr saturated_int(saturated_int const&) noexcept = default;

    constexpr saturated_int& operator=(saturated_int&&) noexcept = default;
    constexpr saturated_int& operator=(saturated_int const&) noexcept = default;

    template<class U>
    constexpr saturated_int(U i) noexcept
    : i(saturated_cast<T>(i))
    {}

    constexpr saturated_int(T i) noexcept
    : i(i)
    {}

    template<class U>
    constexpr saturated_int& operator=(U i) noexcept
    {
        this->i = saturated_cast<T>(i);
        return *this;
    }

    constexpr saturated_int& operator=(T i) noexcept
    {
        this->i = i;
        return *this;
    }

    template<class U, class = std::enable_if_t<detail::is_numeric_convertible<U>::value>>
    constexpr operator U () const noexcept { return saturated_cast<U>(i); }

    constexpr operator T () const noexcept { return this->i; }

    constexpr T underlying() const noexcept { return this->i; }

private:
    T i;
};


// integer type with safe_cast
template<class T>
struct safe_int
{
    using value_type = T;

    constexpr safe_int(safe_int&&) noexcept = default;
    constexpr safe_int(safe_int const&) noexcept = default;

    constexpr safe_int& operator=(safe_int&&) noexcept = default;
    constexpr safe_int& operator=(safe_int const&) noexcept = default;

    template<class U>
    constexpr safe_int(U i) noexcept
    : i(safe_cast<T>(i))
    {}

    constexpr safe_int(T i) noexcept
    : i(i)
    {}

    template<class U>
    constexpr safe_int& operator=(U i) noexcept
    {
        this->i = safe_cast<T>(i);
        return *this;
    }

    constexpr safe_int& operator=(T i) noexcept
    {
        this->i = i;
        return *this;
    }

    template<class U, class = std::enable_if_t<detail::is_numeric_convertible<U>::value>>
    constexpr operator U () const noexcept { return safe_cast<U>(i); }

    constexpr operator T () const noexcept { return this->i; }

    constexpr T underlying() const noexcept { return this->i; }

private:
    T i;
};


template<class T> safe_int(T i) -> safe_int<T>;
template<class T> checked_int(T i) -> checked_int<T>;
template<class T> saturated_int(T i) -> saturated_int<T>;

namespace std
{
    template<class T> struct underlying_type< ::safe_int<T>     > { using type = T; };
    template<class T> struct underlying_type< ::checked_int<T>  > { using type = T; };
    template<class T> struct underlying_type< ::saturated_int<T>> { using type = T; };
}

// Implementation


namespace detail
{
    template<class T, class = void>
    struct underlying_type_or_integral
    { using type = T; };

    template<class T>
    struct underlying_type_or_integral<T, std::enable_if_t<std::is_enum_v<T>>>
    { using type = std::underlying_type_t<T>; };

    template<class T>
    using underlying_type_or_integral_t = typename underlying_type_or_integral<T>::type;

    template<class T>
    struct type_
    {
        using type = T;
    };

    template <class Dst, class Src>
    constexpr int check_int(Src const & /*unused*/) noexcept
    {
        static_assert(is_numeric_convertible<Src>::value, "Argument must be an integral.");
        static_assert(is_numeric_convertible<Dst>::value, "Dst must be an integral.");
        return 1;
    }

    template <class Dst, class Src>
    constexpr Dst checked_cast(type_<Dst> /*unused*/, Src value) noexcept
    {
    #ifndef NDEBUG
        using DstIsSign = std::is_signed<Dst>;
        using SrcIsSign = std::is_signed<Src>;
        using dst_limits = std::numeric_limits<Dst>;
        if constexpr (DstIsSign::value == SrcIsSign::value) {
            assert(dst_limits::max() >= value);
            assert(dst_limits::min() <= value);
        }
        else if constexpr (DstIsSign::value) {
            assert(std::make_unsigned_t<Dst>(dst_limits::max()) >= value);
        }
        else {
            assert(0 <= value);
            assert(dst_limits::max() >= std::make_unsigned_t<Src>(value));
        }
    # endif
        return static_cast<Dst>(value);
    }

    template <class Dst>
    constexpr Dst checked_cast(type_<Dst> /*unused*/, Dst value) noexcept
    {
        return value;
    }

    template <class Dst, class Src>
    constexpr Dst saturated_cast(type_<Dst> /*unused*/, Src value) noexcept
    {
        if constexpr (std::is_signed<Dst>::value == std::is_signed<Src>::value && sizeof(Dst) >= sizeof(Src)) {
            return static_cast<Dst>(value);
        }

        using dst_limits = std::numeric_limits<Dst>;
        Dst const new_max_value = dst_limits::max() < value ? dst_limits::max() : static_cast<Dst>(value);
        if constexpr (!std::is_signed<Dst>::value && std::is_signed<Src>::value) {
            return value < 0 ? Dst{0} : new_max_value;
        }
        if constexpr (!std::is_signed<Src>::value) {
            return new_max_value;
        }
        return dst_limits::min() > value ? dst_limits::min() : new_max_value;
    }

    template <class Dst>
    constexpr Dst saturated_cast(type_<Dst> /*unused*/, Dst value) noexcept
    {
        return value;
    }

    using ull = unsigned long long;
    using ll = long long;

    template<class From, class To>
    struct is_safe_convertible
    {
        using S = std::numeric_limits<From>;
        using D = std::numeric_limits<To>;

        static const bool value
          = (std::is_signed<From>::value ? std::is_signed<To>::value : true)
          && ll(D::min()) <= ll(S::min()) && ull(S::max()) <= ull(D::max())
        ;
    };

    template<class From>
    struct is_safe_convertible<From, From>
    {
        static const bool value = true;
    };
}  // namespace detail


template <class Dst, class Src>
constexpr Dst checked_cast(Src value) noexcept
{
    static_assert(detail::check_int<Dst>(value) );
    using dst_type = detail::underlying_type_or_integral_t<Dst>;
    using src_type = detail::underlying_type_or_integral_t<Src>;
    return static_cast<Dst>(detail::checked_cast(detail::type_<dst_type>{}, static_cast<src_type>(value)));
}


template <class Dst, class Src>
constexpr Dst saturated_cast(Src value) noexcept
{
    static_assert(detail::check_int<Dst>(value) );
    using dst_type = detail::underlying_type_or_integral_t<Dst>;
    using src_type = detail::underlying_type_or_integral_t<Src>;
    return static_cast<Dst>(detail::saturated_cast(detail::type_<dst_type>{}, static_cast<src_type>(value)));
}


template<class From, class To>
using is_safe_convertible = std::integral_constant<bool, detail::is_safe_convertible<
    detail::underlying_type_or_integral_t<From>,
    detail::underlying_type_or_integral_t<To>
>::value>;

template <class Dst, class Src>
constexpr Dst safe_cast(Src value) noexcept
{
    static_assert(detail::check_int<Dst>(value) );
    static_assert(is_safe_convertible<Src, Dst>::value, "Unsafe conversion.");
    return static_cast<Dst>(value);
}
