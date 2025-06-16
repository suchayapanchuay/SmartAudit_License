/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean

   Conversion function between packed and memory aligned memory bitmap format

*/


#pragma once

#include <cstdint>
#include <cstddef>

// pad some number to next upper multiple of 2
constexpr uint16_t align2(uint16_t value) noexcept
{
    return (value+1) & ~1;
}

// pad some number to next upper multiple of 4
constexpr uint16_t align4(uint16_t value) noexcept
{
    return (value+3) & ~3;
}

// compute the number of bytes necessary to hold a given number of bits
constexpr uint8_t nbbytes(unsigned value) noexcept
{
    return static_cast<uint8_t>((value+7) / 8);
}

// even pad some number to next upper even number
constexpr unsigned even_pad_length(unsigned value) noexcept
{
    return value+(value&1);
}

// compute the number of bytes necessary to hold a given number of bits
constexpr uint32_t nbbytes_large(unsigned value) noexcept
{
    return ((value+7) / 8);
}

inline void out_bytes_le(uint8_t * ptr, const uint8_t nb, const unsigned value) noexcept
{
    for (uint8_t b = 0 ; b < nb ; ++b){
        ptr[b] = static_cast<uint8_t>(value >> (8 * b));
    }
}

// this name because the fonction below is only defined for 1 to 4/8 bytes (works on underlying unsigned)
inline unsigned in_uint32_from_nb_bytes_le(const uint8_t nb, const uint8_t * ptr) noexcept
{
    unsigned res = 0;
    for (int b = 0 ; b < nb ; ++b){
        res |= ptr[b] << (8 * b);
    }
    return res;
}

// The  rmemcpy() function copies n bytes from memory area src to memory area dest inverting end and beginning.
// The memory areas must not overlap.
inline void rmemcpy(uint8_t *dest, const uint8_t *src, size_t n) noexcept
{
    for (size_t i = 0; i < n ; i++){
        dest[n-1-i] = src[i];
    }
}

inline void reverseit(uint8_t *buffer, size_t n) noexcept
{
    for (size_t i = 0 ; i < n / 2; i++){
        uint8_t tmp = buffer[n-1-i];
        buffer[n-1-i] = buffer[i];
        buffer[i] = tmp;
    }
}

