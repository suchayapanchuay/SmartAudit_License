/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2014
    Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen
*/

#pragma once

#include "utils/sugar/array_view.hpp"

#include <string>
#include <vector>

#include <cassert>
#include <cstring>


template<class String, class... Strings>
[[nodiscard]] std::string str_concat(String&& str, Strings const&... strs);

template<class... Strings>
[[nodiscard]] std::string str_concat(std::string&& str, Strings const&... strs);

template<class... Strings>
void str_append(std::string& str, Strings const&... strs);

template<class... Strings>
void str_assign(std::string& str, Strings const&... strs);


inline char const* strchr(chars_view str, char pattern) noexcept
{
    // cast to u8 to prevent overflow when char -> int in memchr
    auto c = static_cast<uint8_t>(pattern);
    return static_cast<char const*>(std::memchr(str.data(), c, str.size()));
}

inline char* strchr(writable_chars_view str, char pattern) noexcept
{
    // cast to u8 to prevent overflow when char -> int in memchr
    auto c = static_cast<uint8_t>(pattern);
    return static_cast<char*>(std::memchr(str.data(), c, str.size()));
}

namespace utils
{

inline bool starts_with(chars_view str, chars_view prefix) noexcept
{
    return str.size() >= prefix.size()
        && str.first(prefix.size()).as<std::string_view>() == prefix.as<std::string_view>();
}

inline bool ends_with(chars_view str, chars_view suffix) noexcept
{
    return str.size() >= suffix.size()
        && str.last(suffix.size()).as<std::string_view>() == suffix.as<std::string_view>();
}

/**
 * Copies up to \c buflen - 1 characters from the NUL-terminated string \c src to \c dst, NUL-terminating the result.
 * \return total length of the string they tried to create.
 * \note when \c buflen is 0, no copy is performed
 * @{
 */
std::size_t strlcpy(char* dest, chars_view src, std::size_t buflen) noexcept;
std::size_t strlcpy(char* dest, char const* src, std::size_t buflen) noexcept;

template<std::size_t N>
inline std::size_t strlcpy(char (&dest)[N], chars_view src) noexcept
{
    return strlcpy(dest, src, N);
}

template<std::size_t N>
inline std::size_t strlcpy(char (&dest)[N], char const* src) noexcept
{
    return strlcpy(dest, src, N);
}
/**
 * @}
 */


/**
 * Copies up to \c n - 1 characters from the NUL-terminated string \c src to \c dst, NUL-terminating the result.
 * \return false if \c src is too long
 * @{
 */
[[nodiscard]] inline bool strbcpy(char* dest, chars_view src, std::size_t n) noexcept
{
    return strlcpy(dest, src, n) < n;
}

[[nodiscard]] inline bool strbcpy(char* dest, char const* src, std::size_t n) noexcept
{
    return strlcpy(dest, src, n) < n;
}

template<std::size_t N>
[[nodiscard]] inline bool strbcpy(char (&dest)[N], chars_view src) noexcept
{
    return strlcpy(dest, src, N) < N;
}

template<std::size_t N>
[[nodiscard]] inline bool strbcpy(char (&dest)[N], char const* src) noexcept
{
    return strlcpy(dest, src, N) < N;
}
/**
 * @}
 */

//@{
void str_replace_inplace(std::string& str,
                         chars_view pattern,
                         chars_view replacement);

struct StrReplaceBetweenPattern
{
    StrReplaceBetweenPattern(StrReplaceBetweenPattern&&) = delete;
    StrReplaceBetweenPattern& operator=(StrReplaceBetweenPattern const&) = delete;

    StrReplaceBetweenPattern(chars_view str, char pattern, chars_view replacement);

    ~StrReplaceBetweenPattern()
    {
        operator delete(storage_owned);
    }

    chars_view temporary_chars() const&& noexcept { return {storage_view, storage_len}; }
    chars_view temporary_chars() && noexcept { return {storage_view, storage_len}; }
    chars_view chars() const& noexcept { return {storage_view, storage_len}; }
    chars_view chars() & noexcept { return {storage_view, storage_len}; }

private:
    char* storage_owned = nullptr;
    char const* storage_view;
    std::size_t storage_len;
};
//@}

} // namespace utils

namespace detail
{
    inline chars_view to_string_view_or_char(chars_view av, int /*dummy*/) noexcept
    {
        return av;
    }

    inline chars_view to_string_view_or_char(char const* s, char /*dummy*/) noexcept
    {
        return {s, ::strlen(s)};
    }

    inline char to_string_view_or_char(char c, int /*dummy*/) noexcept
    {
        return c;
    }


    inline std::size_t len_from_av_or_char(chars_view av) noexcept
    {
        return av.size();
    }

    inline std::size_t len_from_av_or_char(char c) noexcept
    {
        (void)c;
        return 1;
    }


    inline char* append_from_av_or_char(char* s, chars_view av) noexcept
    {
        memcpy(s, av.data(), av.size());
        return s + av.size();
    }

    inline char* append_from_av_or_char(char* s, char c) noexcept
    {
        *s = c;
        return s + 1;
    }


    template<class Buffer, class... StringsOrChars>
    void str_concat_view(Buffer& str, StringsOrChars&&... strs) noexcept
    {
        auto ipos = str.size();
        str.resize(str.size() + (... + len_from_av_or_char(strs)));
        auto p = str.data() + ipos;
        (..., void(p = append_from_av_or_char(p, strs)));
    }

#ifndef NDEBUG
    inline char const* get_ptr_data(chars_view av) noexcept
    {
        return av.data();
    }

    inline char const* get_ptr_data(char /*c*/) noexcept
    {
        return nullptr;
    }

    template<class... StringsOrChars>
    inline bool inputs_contains_output(char const* p, StringsOrChars&&... strs) noexcept
    {
        return ((p == get_ptr_data(strs)) || ...);
    }
#endif
} // namespace detail


template<class String, class... Strings>
[[nodiscard]] inline
std::string str_concat(String&& str, Strings const&... strs)
{
    std::string s;
    detail::str_concat_view(s, detail::to_string_view_or_char(str, 1),
                               detail::to_string_view_or_char(strs, 1)...);
    return s;
}

template<class... Strings>
[[nodiscard]] inline
std::string str_concat(std::string&& str, Strings const&... strs)
{
    detail::str_concat_view(str, detail::to_string_view_or_char(strs, 1)...);
    return std::move(str);
}


template<class... Strings>
inline void str_append(std::string& str, Strings const&... strs)
{
    assert(!detail::inputs_contains_output(str.data(), detail::to_string_view_or_char(strs, 1)...));
    detail::str_concat_view(str, detail::to_string_view_or_char(strs, 1)...);
}


template<class... Strings>
inline void str_assign(std::string& str, Strings const&... strs)
{
    assert(!detail::inputs_contains_output(str.data(), detail::to_string_view_or_char(strs, 1)...));

    str.clear();
    detail::str_concat_view(str, detail::to_string_view_or_char(strs, 1)...);
}


template<class... Strings>
inline void str_append(std::vector<char>& str, Strings const&... strs)
{
    assert(!detail::inputs_contains_output(str.data(), detail::to_string_view_or_char(strs, 1)...));
    detail::str_concat_view(str, detail::to_string_view_or_char(strs, 1)...);
}


struct is_blank_fn
{
    bool operator()(char c) const noexcept
    {
        return c == ' ' || c == '\t';
    }
};

namespace detail
{
    template<class Pred>
    const char* ltrim(const char* first, const char* last, Pred&& pred)
    {
        while (first != last && pred(*first)) {
            ++first;
        }
        return first;
    }

    template<class Pred>
    const char* rtrim(const char* first, const char* last, Pred&& pred)
    {
        while (first != last && pred(*(last - 1))) {
            --last;
        }
        return last;
    }

    inline writable_chars_view chars_view_to_writable_chars_view(chars_view av) noexcept
    {
        char* first = const_cast<char*>(av.begin()); /*NOLINT*/
        char* last = const_cast<char*>(av.end()); /*NOLINT*/
        return writable_chars_view{first, last};
    }
} // namespace detail

template<class Pred = is_blank_fn>
chars_view ltrim(chars_view chars, Pred&& pred = Pred()) /*NOLINT*/
{
    return {detail::ltrim(chars.begin(), chars.end(), pred), chars.end()};
}

template<class Pred = is_blank_fn>
chars_view rtrim(chars_view chars, Pred&& pred = Pred()) /*NOLINT*/
{
    return {chars.begin(), detail::rtrim(chars.begin(), chars.end(), pred)};
}

template<class Pred = is_blank_fn>
chars_view trim(chars_view chars, Pred&& pred = Pred()) /*NOLINT*/
{
    auto first = detail::ltrim(chars.begin(), chars.end(), pred);
    auto last = detail::rtrim(first, chars.end(), pred);
    return {first, last};
}

template<class Pred = is_blank_fn>
writable_chars_view ltrim(writable_chars_view chars, Pred&& pred = Pred()) /*NOLINT*/
{
    return detail::chars_view_to_writable_chars_view(ltrim(chars_view(chars), pred));
}

template<class Pred = is_blank_fn>
writable_chars_view rtrim(writable_chars_view chars, Pred&& pred = Pred()) /*NOLINT*/
{
    return detail::chars_view_to_writable_chars_view(rtrim(chars_view(chars), pred));
}

template<class Pred = is_blank_fn>
writable_chars_view trim(writable_chars_view chars, Pred&& pred = Pred()) /*NOLINT*/
{
    return detail::chars_view_to_writable_chars_view(trim(chars_view(chars), pred));
}
