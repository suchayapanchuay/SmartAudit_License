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

#include <cstddef>
#include <type_traits>

#include <string_view>

#include <cassert>

#include "utils/sugar/array_view.hpp"
#include "utils/sugar/flags.hpp"


namespace cfg_desc
{

#define TYPE_REQUIEREMENT(T)                                           \
    static_assert(!std::is_arithmetic_v<T> || std::is_same_v<T, bool>, \
        "T cannot be an arithmetic type, "                             \
        "use types::u8, types::u16, types::s16, etc instead, "         \
        "eventually types::int_ or types::unsigned_")


#define MK_ENUM_OP(T)                                                     \
    constexpr T operator | (T x, T y)                                     \
    { return static_cast<T>(static_cast<int>(x) | static_cast<int>(y)); } \
    constexpr T operator & (T x, T y)                                     \
    { return static_cast<T>(static_cast<int>(x) & static_cast<int>(y)); } \
    constexpr T operator ~ (T x)                                          \
    { return static_cast<T>(~static_cast<int>(x)); }                      \
    constexpr T& operator |= (T& x, T y) { x = x | y; return x; }         \
    constexpr T& operator &= (T& x, T y) { x = x & y; return x; }


enum class Tag : unsigned
{
    Perf,
    Debug,
    Workaround,
    Compatibility,
    MAX,
};

inline std::string_view tag_to_sv(Tag tag)
{
    using namespace std::string_view_literals;
    switch (tag) {
        case Tag::Perf: return "perf"sv;
        case Tag::Debug: return "debug"sv;
        case Tag::Workaround: return "workaround"sv;
        case Tag::Compatibility: return "compatibility"sv;
        case Tag::MAX:;
    }
    return {};
}

}

template<>
struct utils::enum_as_flag<cfg_desc::Tag>
{
    static constexpr std::size_t max = std::size_t(cfg_desc::Tag::MAX);
};

namespace cfg_desc
{
using Tags = utils::flags_t<Tag>;

enum class DestSpecFile : uint8_t
{
    none         = 0,
    ini_only     = 1 << 0,
    global_spec  = 1 << 1,
    vnc          = 1 << 2,
    rdp          = 1 << 3,
    // References:
    // * SOG-IS Crypto Evaluation Scheme – Agreed Cryptographic Mechanisms, Version 1.3 (February 2023)
    //   https://www.sogis.eu/documents/cc/crypto/SOGIS-Agreed-Cryptographic-Mechanisms-1.3.pdf
    // * BSI document TR-02102-2: "Cryptographic Mechanisms: Recommandations and Key Lengths: Use of Transport Layer Security (TLS)", Version 2023-1
    //   https://www.bsi.bund.de/SharedDocs/Downloads/EN/BSI/Publications/TechGuidelines/TG02102/BSI-TR-02102-2.html
    // * OpenSSL Ciphers:
    //   https://www.openssl.org/docs/manmaster/man1/openssl-ciphers.html
    // Some algos are excluded from this list because they are not used by the
    // proxy (DHE-PSK-*, DHE-DSS-*, ...) or where not included in documentation
    // submited by WALLIX for BSZ certification (*-CCM)
    rdp_sogisces_1_3_2030 = 1 << 4,
};
MK_ENUM_OP(DestSpecFile)

constexpr bool contains_conn_policy(DestSpecFile dest)
{
    return bool(dest & ~(DestSpecFile::global_spec | DestSpecFile::ini_only));
}


enum class ResetBackToSelector : bool { No, Yes };

enum class Loggable : uint8_t { No, Yes, OnlyWhenContainsPasswordString, };


struct names
{
    // name for all field
    std::string_view all;
    // field name in ini file
    std::string_view ini {};
    // field name between proxy <-> acl (default is ${section}::${name})
    std::string_view acl {};
    // field name in connection policy
    std::string_view connpolicy {};
    // field name displayed in connection policy and ini (info in spec file)
    std::string_view display {};

    // names with acl = all
    static names acl_shortname(std::string_view all_and_acl)
    {
        return {
            .all = all_and_acl,
            .acl = all_and_acl,
        };
    }

    std::string_view cpp_name() const { assert(!all.empty()); return all; }
    std::string_view ini_name() const { return ini.empty() ? all : ini; }
    std::string_view acl_name() const { return acl.empty() ? all : acl; }
    std::string_view connpolicy_name() const { return connpolicy.empty() ? all : connpolicy; }
};

namespace impl
{
    struct integer_base {};
    struct signed_base : integer_base {};
    struct unsigned_base : integer_base {};
}

namespace types
{
    struct u8 : impl::unsigned_base {};
    struct u16 : impl::unsigned_base {};
    struct u32 : impl::unsigned_base {};
    struct u64 : impl::unsigned_base {};

    struct i8 : impl::signed_base {};
    struct i16 : impl::signed_base {};
    struct i32 : impl::signed_base {};
    struct i64 : impl::signed_base {};

    struct unsigned_ : impl::unsigned_base {};
    struct int_ : impl::signed_base {};

    struct rgb {};

    template<unsigned Len> struct fixed_string {};
    template<unsigned Len> struct fixed_binary {};

    template<class T>
    struct list
    {
        TYPE_REQUIEREMENT(T);
    };

    struct ip_string {};

    struct dirpath {};

    template<class T, long min, long max>
    struct range
    {
        TYPE_REQUIEREMENT(T);
    };

    template<class T>
    struct mebibytes
    {
        static_assert(std::is_base_of_v<impl::unsigned_base, T>);
    };

    struct performance_flags {};
}

namespace cpp
{
    struct expr { char const * value; };
    #define CPP_EXPR(expression) ::cfg_desc::cpp::expr{#expression}
}



enum class SpecAttributes : uint16_t
{
    none            = 0,
    logged          = 1 << 0,
    hex             = 1 << 1,
    advanced        = 1 << 2,
    iptables        = 1 << 3,
    password        = 1 << 4,
    image           = 1 << 5,
    external        = 1 << 6,
    adminkit        = 1 << 7,
};

MK_ENUM_OP(SpecAttributes)


enum class SesmanIO : uint8_t
{
    no_acl       = 0,
    acl_to_proxy = 1 << 0,
    proxy_to_acl = 1 << 1,
    rw           = acl_to_proxy | proxy_to_acl,
};

MK_ENUM_OP(SesmanIO)

struct SpecInfo
{
    DestSpecFile dest;
    SesmanIO acl_io;
    SpecAttributes attributes;
    ResetBackToSelector reset_back_to_selector;
    Loggable loggable;
    std::string_view image_path;

    bool has_spec() const
    {
        return bool(dest);
    }

    bool has_ini() const
    {
        return has_spec() && !bool(attributes & SpecAttributes::external);
    }

    bool has_acl() const
    {
        return bool(acl_io);
    }

    bool has_global_spec() const
    {
        return bool(dest & DestSpecFile::global_spec);
    }

    bool has_connpolicy() const
    {
        return contains_conn_policy(dest);
    }
};

struct SesmanInfo
{
    SesmanIO acl_io;
    ResetBackToSelector reset_back_to_selector;
    Loggable loggable;

    operator SpecInfo () const
    {
        return {
            DestSpecFile::none,
            acl_io,
            SpecAttributes::none,
            reset_back_to_selector,
            loggable,
            std::string_view(),
        };
    }
};

template<bool has_image, bool is_compatible_connpolicy>
struct CheckedSpecAttributes
{
    SpecAttributes attr;
    std::string_view image_path {};

    template<bool has_image2, bool is_compatible_connpolicy2>
    constexpr CheckedSpecAttributes<
        has_image || has_image2,
        is_compatible_connpolicy || is_compatible_connpolicy2
    >
    operator | (CheckedSpecAttributes<has_image2, is_compatible_connpolicy2> other) const
    {
        static_assert(!(has_image && has_image2), "Image specified twice");

        return {
            attr | other.attr,
            has_image ? image_path : other.image_path,
        };
    }
};

namespace spec
{
    constexpr inline CheckedSpecAttributes<false, true> hex {SpecAttributes::hex};
    constexpr inline CheckedSpecAttributes<false, true> advanced {SpecAttributes::advanced};
    constexpr inline CheckedSpecAttributes<false, false> iptables {SpecAttributes::iptables};
    constexpr inline CheckedSpecAttributes<false, true> password {SpecAttributes::password};
    constexpr inline CheckedSpecAttributes<false, false> adminkit {SpecAttributes::adminkit};
    constexpr inline CheckedSpecAttributes<false, false> logged {SpecAttributes::logged};

    constexpr CheckedSpecAttributes<true, false> image(std::string_view image_path)
    {
        return {SpecAttributes::image, image_path};
    }


    constexpr inline SesmanInfo no_acl {SesmanIO::no_acl, ResetBackToSelector::No, Loggable::No};

    constexpr SesmanInfo acl_to_proxy(ResetBackToSelector reset_back_to_selector, Loggable loggable)
    {
        return {SesmanIO::acl_to_proxy, reset_back_to_selector, loggable};
    }

    constexpr SesmanInfo proxy_to_acl(ResetBackToSelector reset_back_to_selector)
    {
        return {SesmanIO::proxy_to_acl, reset_back_to_selector, Loggable::No};
    }

    constexpr SesmanInfo acl_rw(ResetBackToSelector reset_back_to_selector, Loggable loggable)
    {
        return {SesmanIO::rw, reset_back_to_selector, loggable};
    }


    template<bool is_compatible_connpolicy = true>
    constexpr SpecInfo ini_only(SesmanInfo acl)
    {
        return {
            DestSpecFile::ini_only,
            acl.acl_io,
            SpecAttributes(),
            acl.reset_back_to_selector,
            acl.loggable,
            std::string_view(),
        };
    }

    template<bool has_image = false, bool is_compatible_connpolicy = true>
    constexpr SpecInfo global_spec(
        SesmanInfo acl,
        CheckedSpecAttributes<has_image, is_compatible_connpolicy> checked_attr = {})
    {
        return {
            DestSpecFile::global_spec,
            acl.acl_io,
            checked_attr.attr,
            acl.reset_back_to_selector,
            acl.loggable,
            checked_attr.image_path,
        };
    }

    template<bool has_image = false, bool is_compatible_connpolicy = true>
    constexpr SpecInfo external(
        CheckedSpecAttributes<has_image, is_compatible_connpolicy> checked_attr = {})
    {
        return {
            DestSpecFile::global_spec,
            SesmanIO(),
            checked_attr.attr | SpecAttributes::external,
            ResetBackToSelector::No,
            Loggable::No,
            checked_attr.image_path,
        };
    }

    constexpr SpecInfo acl_connpolicy(
        DestSpecFile dest,
        CheckedSpecAttributes<false, true> checked_attr = {})
    {
        return {
            dest,
            SesmanIO(),
            checked_attr.attr | SpecAttributes::external,
            ResetBackToSelector::No,
            Loggable::No,
            std::string_view(),
        };
    }

    constexpr SpecInfo connpolicy(
        DestSpecFile dest,
        Loggable loggable,
        CheckedSpecAttributes<false, true> checked_attr = {})
    {
        return {
            dest,
            SesmanIO::acl_to_proxy,
            checked_attr.attr,
            ResetBackToSelector::No,
            loggable,
            std::string_view(),
        };
    }
}

}
