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

#include "capture/mwrm3.hpp"
#include "transport/crypto_transport.hpp"
#include "utils/sugar/scope_exit.hpp"
#include "utils/hexadecimal_string_to_buffer.hpp"
#include "utils/move_remaining_data.hpp"
#include "cxx/compiler_version.hpp"

#include <iostream>
#include <string_view>

namespace
{

struct TypeName : std::string_view {};

#ifdef __clang__
#if REDEMPTION_COMP_CLANG_LIKE < 1600
// clang < 16: (anonymous namespace)::TypeName (anonymous namespace)::get_type_name() [T = E]
constexpr std::size_t get_type_prefix_length = 76;
#else
// clang >= 16: TypeName (anonymous namespace)::get_type_name() [T = E]
constexpr std::size_t get_type_prefix_length = 53;
#endif
#endif

template<class T>
constexpr TypeName get_type_name()
{
    std::string_view s = __PRETTY_FUNCTION__;
    s.remove_suffix(1);
#ifdef __clang__
    s.remove_prefix(get_type_prefix_length);
#elif defined(__GNUG__)
    s.remove_prefix(71);
#endif
    return {s};
}

template<class E, E e>
constexpr TypeName get_enum_name()
{
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 9)
    std::string_view s = __PRETTY_FUNCTION__;
#ifdef __clang__
    s.remove_prefix(get_type_prefix_length + get_type_name<E>().size() + 6);
    if ('0' <= s[0] && s[0] <= '9')
    {
        return {};
    }
#elif defined(__GNUG__)
    s.remove_prefix(71 + get_type_name<E>().size() + 8);
    if (s[0] == '(')
    {
        return {};
    }
#endif
    s.remove_prefix(get_type_name<E>().size() + 2);
    s.remove_suffix(1);
    return {s};
#else
    return {};
#endif
}


template<class E>
class enum_names
{
    constexpr static std::size_t _len(std::index_sequence<> /*ints*/)
    {
        return 0;
    }

    template<std::size_t... ints>
    constexpr static std::size_t _len(std::index_sequence<ints...> /*ints*/)
    {
        std::string_view names[]{get_enum_name<E, E(ints)>()...};
        for (auto& name : names)
        {
            if (name.empty())
            {
                return static_cast<std::size_t>(&name - names);
            }
        }
        return 0;
    }

    template<std::size_t... ints>
    constexpr static auto _names(std::index_sequence<ints...> /*ints*/)
    {
        return std::array<std::string_view, sizeof...(ints)>{
            get_enum_name<E, E(ints)>()...
        };
    }

    template<std::size_t... ints>
    constexpr static auto _name(E e, std::index_sequence<ints...> /*ints*/)
    {
        auto v = +underlying_cast(e);
        std::string_view sv;
        ((v == ints ? void(sv = get_enum_name<E, E(ints)>()) : void()), ...);
        return sv;
    }

public:
    static constexpr unsigned char size
        = _len(std::make_index_sequence<(sizeof(E) <= sizeof(uint16_t) ? 126 : 0)>());

    static std::string_view name(E e)
    {
        if constexpr (size > 0)
        {
            return _name(e, std::make_index_sequence<size>());
        }
        else
        {
            (void)e;
            return {};
        }
    }
};

template<class T>
void print_enum_value(T x, std::string_view name)
{
    if (not name.empty())
    {
        std::cout << name << "(" << x << ")";
    }
    else
    {
        std::cout << x;
    }
}

template<class E, E e>
void print_enum()
{
    print_enum_value(+underlying_cast(e), get_enum_name<Mwrm3::Type, e>());
}

template<class E>
void print_enum(E e)
{
    print_enum_value(+underlying_cast(e), enum_names<E>::name(e));
}

template<class T, class = void>
struct print_value_impl
{
    static_assert(!std::is_same<T, T>::value, "missing specialization or not a regular type (struct with bytes or str, enum or integral)");
};

template<class T>
void print_value(T const& x)
{
    if constexpr (std::is_enum_v<T>)
    {
        print_enum(x);
    }
    else if constexpr (std::is_integral_v<T>)
    {
        std::cout << x;
    }
    else
    {
        print_value_impl<T>::print(x);
    }
}


constexpr inline char const* hexadecimal_string = "0123456789ABCDEF";

void print_bytes(bytes_view bytes)
{
    if (bytes.empty())
    {
        return ;
    }

    std::cout << "\\x";
    for (auto byte : bytes)
    {
        std::cout << hexadecimal_string[byte >> 4];
        std::cout << hexadecimal_string[byte & 0xf];
    }
}

template<class T>
struct print_value_impl<T, decltype(void(std::declval<T&>().bytes))>
{
    static void print(T const& x)
    {
        print_bytes(x.bytes);
    }
};


void print_chars_view(chars_view chars)
{
    std::cout.write(chars.data(), chars.size());
}

template<class T>
struct print_value_impl<T, decltype(void(std::declval<T&>().str))>
{
    static void print(T const& x)
    {
        print_chars_view(x.str);
    }
};


template<class Rep, class Period>
struct print_value_impl<std::chrono::duration<Rep, Period>, void>
{
    static void print(std::chrono::duration<Rep, Period> const& duration)
    {
        // C++20: std::cout << duration;
        std::cout << duration.count();

        using Duration = std::chrono::duration<Rep, Period>;
        if constexpr (std::is_same_v<Duration, std::chrono::seconds>)
        {
            std::cout << "s";
        }
        else
        {
            static_assert(!std::is_same<Duration, Duration>::value, "missing suffix");
        }
    }
};


int mwrm3_text_viewer(InCryptoTransport& crypto_transport)
{
    int nb_packet = 0;

    std::array<uint8_t, 1024*16> buffer;
    auto const consumed = crypto_transport.partial_read(buffer);
    bytes_view remaining_data = make_array_view(buffer).first(consumed);

    auto print_values = [&](auto type, bytes_view next_data, auto... xs){
        ++nb_packet;
        remaining_data = next_data;

        std::cout << "#" << nb_packet << "\nMwrm3::Type: "; print_enum<Mwrm3::Type, type.value>();
        ((
            std::cout << "\n" << get_type_name<decltype(xs)>() << ": ",
            print_value(xs)
        ), ...);
        std::cout << "\n\n";
    };

    for (;;) switch (Mwrm3::parse_packet(remaining_data, print_values))
    {
        case Mwrm3::ParserResult::Ok:
            break;

        case Mwrm3::ParserResult::UnknownType: {
            auto int_type = InStream(remaining_data).in_uint16_le();
            std::cerr << "Unknown type: 0x" << std::hex << int_type << "\n";
            return 1;
        }

        case Mwrm3::ParserResult::NeedMoreData: {
            auto reader = [&](writable_bytes_view data){ return crypto_transport.partial_read(data); };
            auto result = move_remaining_data(remaining_data, buffer, reader);
            switch (result.status)
            {
                case RemainingDataResult::Status::Ok:
                    remaining_data = result.data;
                    break;

                case RemainingDataResult::Status::Eof:
                    return 0;

                case RemainingDataResult::Status::DataToLarge:
                    std::cerr << "Data too large\n";
                    return 2;
            }
        }
    }
}


// Reader

template<Mwrm3::Type Type, class... xs>
struct mwrm3_type_info
{
    static const Mwrm3::Type mwrm3_type = Type;
};

template<class... Mwrm3TypeInfo>
struct integral_mwrm3_type_info_list
{
    integral_mwrm3_type_info_list() = default;

    template<class... Us>
    integral_mwrm3_type_info_list(integral_mwrm3_type_info_list<Us...> /*infos*/) noexcept
    {}};
} // anonymous namespace

namespace {
    template<>
    struct integral_mwrm3_type_info_list<>
    {};
}

namespace std
{
    template<class... xs, class... ys>
    struct common_type<integral_mwrm3_type_info_list<xs...>, integral_mwrm3_type_info_list<ys...>>
    {
        using type = integral_mwrm3_type_info_list<xs..., ys...>;
    };
}

namespace
{

template<class T>
bool read_choice(T& x)
{
    while (!(std::cin >> x))
    {
        if (std::cin.eof())
        {
            return false;
        }

        std::cin.clear();
        std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n');
    }

    return true;
}



template<Mwrm3::Type type, class... Ts>
void mwrm3_text_writer_print_type(mwrm3_type_info<type, Ts...> /*infos*/)
{
    auto name = get_enum_name<Mwrm3::Type, type>();
    if (not name.empty())
    {
        std::cerr << name;
    }
    else
    {
        std::cerr << "Mwrm3::Type";
    }
    std::cerr << "(" << +underlying_cast(type) << ") --";
    ((std::cerr << "  " << get_type_name<Ts>()), ...);
    std::cerr << "\n";
}

template<class T, class>
struct reader_int
{
    T value;

    bool read()
    {
        return read_choice(this->value);
    }
};

template<class T, class = void>
struct reader_impl
{
    static_assert(!std::is_same<T, T>::value, "missing specialization or not a regular type (struct with bytes or str, enum or integral)");
};

template<class T>
auto reader()
{
    if constexpr (std::is_enum_v<T>)
    {
        auto e = +underlying_cast(T{});
        return reader_int<decltype(e), T>();
    }
    else if constexpr (std::is_integral_v<T>)
    {
        return reader_int<decltype(+T{}), T>();
    }
    else
    {
        return reader_impl<T>();
    }
}

template<class T>
struct reader_impl<T, decltype(void(std::declval<T&>().bytes))>
{
    std::string value;

    bool read()
    {
        for (;;)
        {
            std::cerr << "hexadecimal (" << T::static_size << " bytes)\n";

            if (!read_choice(this->value))
            {
                return false;
            }

            if (T::static_size * 2 == this->value.size()
             && hexadecimal_string_to_buffer(chars_view(value), writable_bytes_view(value)))
            {
                value.erase(T::static_size);
                return true;
            }
        }
        return false;
    }
};

template<class T>
struct reader_impl<T, decltype(void(std::declval<T&>().str))>
{
    std::string value;

    bool read()
    {
        return read_choice(this->value);
    }
};

template<class Rep, class Period>
struct reader_impl<std::chrono::duration<Rep, Period>, void>
{
    using Duration = std::chrono::duration<Rep, Period>;

    Duration value;

    bool read()
    {
        Rep duration;
        if (read_choice(duration))
        {
            value = Duration(duration);
            return true;
        }
        return false;
    }
};

struct PrintSerialization
{
    template<class... Bytes>
    void operator()(Mwrm3::Type /*type*/, Bytes... bytes)
    {
        ((std::cout.write(bytes.as_chars().data(), bytes.as_chars().size())), ...);
        std::cout.flush();
    }
};

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wcomma")
template<Mwrm3::Type type, class... Ts>
void mwrm3_text_writer_read_data(mwrm3_type_info<type, Ts...> m)
{
    mwrm3_text_writer_print_type(m);

    struct Readers : decltype(reader<Ts>())... {};
    Readers readers;

    bool ok = true;
    ((ok ? (
        void(std::cerr << get_type_name<Ts>() << ": "),
        void(ok = static_cast<decltype(reader<Ts>())&>(readers).read())
    ) : void()), ...);

    if (ok)
    {
        Mwrm3::serial_for_type(type, [&](auto serial){
            if constexpr (serial.type == type)
            {
                serial.serialize(Ts(static_cast<decltype(reader<Ts>())&>(readers).value)...,
                    PrintSerialization());
            }
        }, [](auto /*dummy*/){});
    }
}
REDEMPTION_DIAGNOSTIC_POP()

template<class... Mwrm3TypeInfo>
void mwrm3_text_writer_impl(integral_mwrm3_type_info_list<Mwrm3TypeInfo...> /*infos*/)
{
    int nb = 0;

    unsigned i;

    for (;;)
    {
        i = 0;
        ((
            void(std::cerr << i << ": "),
            mwrm3_text_writer_print_type(Mwrm3TypeInfo{}),
            void(++i)
        ), ...);

        unsigned id = 0;
        if (!read_choice(id))
        {
            break;
        }

        if (id >= sizeof...(Mwrm3TypeInfo))
        {
            continue;
        }

        std::cerr << "\n#" << nb << "\n";
        i = 0;
        ((
            void(i == id ? static_cast<void>(mwrm3_text_writer_read_data(Mwrm3TypeInfo{})) : void()),
            void(++i)
        ), ...);
    }
}

void mwrm3_text_writer()
{
    auto bind_params = [](auto type, bytes_view /*remaining*/, auto... xs){
        return integral_mwrm3_type_info_list<mwrm3_type_info<type.value, decltype(xs)...>>();
    };

    mwrm3_text_writer_impl(Mwrm3::unserialize_packet(
        Mwrm3::Type::None, {}, bind_params, [](Mwrm3::Type /*type*/){
            return integral_mwrm3_type_info_list<>();
        }
    ));
}

} // anonymous namespace

int main(int ac, char** av)
{
    if (ac != 2)
    {
        mwrm3_text_writer();
        return 0;
    }

    if (av[1][0] == '-' && av[1][1] == 'h')
    {
        std::cerr << "Usage: mwrm3_editor [file.mwrm3]\n";
        std::cerr << "Example:\n";
        std::cerr << R"(echo -e "1\n1\n1\naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" | mwrm3_editor | mwrm3_editor /dev/stdin)";
        std::cerr << "\n";
        return 1;
    }

    CryptoContext cctx;

    try
    {
        InCryptoTransport infile(cctx, InCryptoTransport::EncryptionMode::Auto);
        infile.open(av[1]);

        return mwrm3_text_viewer(infile);
    }
    catch (Error const& e)
    {
        std::cerr << e.errmsg() << "\n";
        return 1;
    }
}
