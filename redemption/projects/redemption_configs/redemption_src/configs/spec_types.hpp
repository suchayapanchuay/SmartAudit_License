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
*   Copyright (C) Wallix 2010-2015
*   Author(s): Jonathan Poelen
*/

#pragma once

#include <limits>
#include <string>
#include <array>
#include <chrono>
#include <type_traits>
#include <iosfwd>

#include "utils/sugar/zstring_view.hpp"


namespace configs
{

template<class> struct spec_type {};

namespace spec_types
{
    class fixed_binary;
    class fixed_string;
    template<class T> class list;
    using ip = std::string;

    template<class T>
    struct underlying_type_for_range
    {
        using type = T;
    };

    template<class Rep, class Period>
    struct underlying_type_for_range<std::chrono::duration<Rep, Period>>
    {
        using type = Rep;
    };

    template<class T>
    using underlying_type_for_range_t = typename underlying_type_for_range<T>::type;

    template<class T, underlying_type_for_range_t<T> min, underlying_type_for_range_t<T> max>
    struct range {};

    struct rgb
    {
        explicit rgb(uint32_t color = 0) noexcept : rgb_(color) { }

        uint8_t red() const noexcept { return static_cast<uint8_t>(rgb_ >> 16); }
        uint8_t green() const noexcept { return static_cast<uint8_t>((rgb_ >> 8) & 0xff); }
        uint8_t blue() const noexcept { return static_cast<uint8_t>(rgb_ & 0xff); }

        uint32_t to_rgb888() const { return rgb_; }

    private:
        uint32_t rgb_;
    };

    struct directory_path
    {
        directory_path()
        : path("./")
        {}

        directory_path(std::string path)
        : path(std::move(path))
        {
            this->normalize();
        }

        directory_path(char const * path)
        : path(path)
        {
            this->normalize();
        }

        directory_path(zstring_view path)
        : path(path.begin(), path.end())
        {
            this->normalize();
        }

        directory_path(directory_path &&) = default;
        directory_path(directory_path const &) = default;

        directory_path & operator = (std::string new_path)
        {
            this->path = std::move(new_path);
            this->normalize();
            return *this;
        }

        directory_path & operator = (char const * new_path)
        {
            this->path = new_path;
            this->normalize();
            return *this;
        }

        directory_path & operator = (chars_view new_path)
        {
            this->path.assign(new_path.begin(), new_path.end());
            this->normalize();
            return *this;
        }

        directory_path & operator = (directory_path &&) = default;
        directory_path & operator = (directory_path const &) = default;

        [[nodiscard]] char const * c_str() const noexcept { return this->path.c_str(); }

        [[nodiscard]] std::string const & as_string() const noexcept { return this->path; }

    private:
        void normalize()
        {
            if (this->path.empty()) {
                this->path = "./";
            }
            else {
                if (this->path.front() != '/') {
                    if (this->path[0] != '.' || this->path[1] != '/') {
                        this->path.insert(0, "./");
                    }
                }
                if (this->path.back() != '/') {
                    this->path += '/';
                }
            }
        }

        std::string path;
    };

    inline bool operator == (directory_path const & x, directory_path const & y)
    { return x.as_string() == y.as_string(); }

    inline bool operator == (std::string const & x, directory_path const & y)
    { return x == y.as_string(); }

    inline bool operator == (char const * x, directory_path const & y)
    { return x == y.as_string(); }

    inline bool operator == (directory_path const & x, std::string const & y)
    { return x.as_string() == y; }

    inline bool operator == (directory_path const & x, char const * y)
    { return x.as_string() == y; }

    inline bool operator != (directory_path const & x, directory_path const & y)
    { return !(x == y); }

    inline bool operator != (std::string const & x, directory_path const & y)
    { return !(x == y); }

    inline bool operator != (char const * x, directory_path const & y)
    { return !(x == y); }

    inline bool operator != (directory_path const & x, std::string const & y)
    { return !(x == y); }

    inline bool operator != (directory_path const & x, char const * y)
    { return !(x == y); }

    template<class Ch, class Tr>
    std::basic_ostream<Ch, Tr> & operator << (std::basic_ostream<Ch, Tr> & out, directory_path const & path)
    { return out << path.as_string(); }
} // namespace spec_types

} // namespace configs
