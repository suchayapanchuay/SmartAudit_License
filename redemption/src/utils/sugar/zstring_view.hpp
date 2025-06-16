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
Copyright (C) Wallix 2010-2019
Author(s): Jonathan Poelen
*/

#pragma once

#include "utils/sugar/array_view.hpp"
#include "utils/traits/is_null_terminated.hpp"
#include "utils/sugar/std_stream_proto.hpp"
#include <cstddef>
#include <string>
#include <string_view>
#include <cassert>


struct zstring_view
{
    using value_type = char;
    using iterator = char const*;
    using const_iterator = char const*;

    zstring_view() = default;
    zstring_view(zstring_view &&) = default;
    zstring_view(zstring_view const &) = default;
    zstring_view & operator=(zstring_view &&) = default;
    zstring_view & operator=(zstring_view const &) = default;

    constexpr zstring_view(std::nullptr_t) noexcept
    {}

    explicit zstring_view(char const* s) noexcept = delete;

    zstring_view(std::string const& s) noexcept
    : s(s.c_str())
    , len(s.size())
    {}

    template<class String,
        class = std::enable_if_t<is_null_terminated_v<std::remove_reference_t<String>>>>
    zstring_view(String&& s) noexcept(noexcept(s.c_str()) && noexcept(s.size()))
    : s(s.c_str())
    , len(s.size())
    {}

    static constexpr zstring_view from_null_terminated(char const* s, std::size_t n) noexcept
        REDEMPTION_ATTRIBUTE_NONNULL_ARGS
    {
        return zstring_view(s, n);
    }

    static constexpr zstring_view from_null_terminated(chars_view str) noexcept
    {
        return zstring_view(str.data(), str.size());
    }

    static constexpr zstring_view from_null_terminated(char const* s) noexcept
        REDEMPTION_ATTRIBUTE_NONNULL_ARGS
    {
        return from_null_terminated(std::string_view(s));
    }

    [[nodiscard]] constexpr char const* c_str() const noexcept REDEMPTION_ATTRIBUTE_RETURNS_NONNULL
    { return data(); }

    // TODO temporary ?
    constexpr operator char const* () const noexcept REDEMPTION_ATTRIBUTE_RETURNS_NONNULL
    { return c_str(); }

    [[nodiscard]] constexpr char const * data() const noexcept REDEMPTION_ATTRIBUTE_RETURNS_NONNULL
    { return this->s; }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return this->len; }

    constexpr char operator[](std::size_t i) const noexcept
    {
        assert(i <= len);
        return s[i];
    }

    [[nodiscard]] constexpr bool empty() const noexcept { return !this->len; }

    [[nodiscard]] constexpr char const * begin() const noexcept REDEMPTION_ATTRIBUTE_NONNULL_ARGS
    { return this->data(); }
    [[nodiscard]] constexpr char const * end() const noexcept REDEMPTION_ATTRIBUTE_NONNULL_ARGS
    { return this->data() + this->size(); }

    constexpr void pop_front() noexcept
    {
        assert(len);
        ++s;
        --len;
    }

    [[nodiscard]] constexpr std::string_view to_sv() const noexcept
    {
        return std::string_view(c_str(), size());
    }

    [[nodiscard]] constexpr chars_view to_av() const noexcept
    {
        return chars_view(c_str(), size());
    }

    [[nodiscard]] constexpr chars_view to_av_with_null_terminated() const noexcept
    {
        return chars_view(c_str(), size() + 1);
    }

    [[nodiscard]] std::string to_string() const
    {
        return std::string(c_str(), size());
    }

private:
    constexpr zstring_view(char const* s, std::size_t n) noexcept REDEMPTION_ATTRIBUTE_NONNULL_ARGS
    : s(s)
    , len(n)
    {
        assert(s[len] == 0);
    }

    char const* s = "";
    std::size_t len = 0;
};


template<>
inline constexpr bool is_null_terminated_v<zstring_view> = true;


constexpr bool operator==(zstring_view const& lhs, zstring_view const& rhs) noexcept
{
    return lhs.to_sv() == rhs.to_sv();
}

constexpr bool operator==(char const* lhs, zstring_view const& rhs) noexcept
{
    return std::string_view(lhs) == rhs.to_sv();
}

constexpr bool operator==(zstring_view const& lhs, char const* rhs) noexcept
{
    return lhs.to_sv() == std::string_view(rhs);
}

constexpr bool operator!=(zstring_view const& lhs, zstring_view const& rhs) noexcept
{
    return lhs.to_sv() != rhs.to_sv();
}

constexpr bool operator!=(char const* lhs, zstring_view const& rhs) noexcept
{
    return std::string_view(lhs) != rhs.to_sv();
}

constexpr bool operator!=(zstring_view const& lhs, char const* rhs) noexcept
{
    return lhs.to_sv() != std::string_view(rhs);
}

constexpr bool operator<(zstring_view const& lhs, zstring_view const& rhs) noexcept
{
    return lhs.to_sv() < rhs.to_sv();
}

constexpr bool operator<(char const* lhs, zstring_view const& rhs) noexcept
{
    return std::string_view(lhs) < rhs.to_sv();
}

constexpr bool operator<(zstring_view const& lhs, char const* rhs) noexcept
{
    return lhs.to_sv() < std::string_view(rhs);
}

constexpr bool operator<=(zstring_view const& lhs, zstring_view const& rhs) noexcept
{
    return lhs.to_sv() <= rhs.to_sv();
}

constexpr bool operator<=(char const* lhs, zstring_view const& rhs) noexcept
{
    return std::string_view(lhs) <= rhs.to_sv();
}

constexpr bool operator<=(zstring_view const& lhs, char const* rhs) noexcept
{
    return lhs.to_sv() <= std::string_view(rhs);
}

constexpr bool operator>(zstring_view const& lhs, zstring_view const& rhs) noexcept
{
    return lhs.to_sv() > rhs.to_sv();
}

constexpr bool operator>(char const* lhs, zstring_view const& rhs) noexcept
{
    return std::string_view(lhs) > rhs.to_sv();
}

constexpr bool operator>(zstring_view const& lhs, char const* rhs) noexcept
{
    return lhs.to_sv() > std::string_view(rhs);
}

constexpr bool operator>=(zstring_view const& lhs, zstring_view const& rhs) noexcept
{
    return lhs.to_sv() >= rhs.to_sv();
}

constexpr bool operator>=(char const* lhs, zstring_view const& rhs) noexcept
{
    return std::string_view(lhs) >= rhs.to_sv();
}

constexpr bool operator>=(zstring_view const& lhs, char const* rhs) noexcept
{
    return lhs.to_sv() >= std::string_view(rhs);
}

// TODO sized_zstring_view<N>
constexpr zstring_view operator ""_zv(char const * s, std::size_t len) noexcept
{
    return zstring_view::from_null_terminated(s, len);
}

REDEMPTION_OSTREAM(out, zstring_view const& str)
{
    return out << str.to_sv();
}
