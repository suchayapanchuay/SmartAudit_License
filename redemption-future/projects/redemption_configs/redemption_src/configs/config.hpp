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
    Author(s): Christophe Grosjean, Jonathan Poelen, Raphael Zhou, Meng Tan

    Configuration file,
    parsing config file rdpproxy.ini
*/

#pragma once

#include "configs/spec_types.hpp"

#include "utils/cfgloader.hpp"
#include "utils/sugar/array_view.hpp"
#include "utils/sugar/bytes_view.hpp"
#include "utils/sugar/numerics/safe_conversions.hpp"

#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>

// config members
//@{
#include "core/app_path.hpp"
#include "core/RDP/rdp_performance_flags.hpp"
#include "utils/file_permissions.hpp"
#include "utils/screen_resolution.hpp"
#include <string>
#include <chrono>
#include <vector>
//@}

#include "config_variant.hpp"
#include "configs/loggable.hpp"
#include "configs/acl_proxy_communication_mask.hpp"

#include "configs/autogen/enums.hpp"
#include "configs/autogen/variables_configuration_fwd.hpp"
#include "configs/autogen/variables_configuration.hpp"

#include "configs/autogen/max_str_buffer_size.hpp"


class Inifile : private noncopyable
{
public:
    // enum class authid_t : unsigned;
    using authid_t = ::configs::authid_t;

    explicit Inifile()
    {
        this->initialize();
    }

    template<class T>
    [[nodiscard]] typename T::type const & get() const noexcept
    {
        return static_cast<T const &>(this->variables).value;
    }

    template<class T>
    [[nodiscard]] typename T::type & get_mutable_ref() noexcept
    {
        static_assert(!(T::acl_proxy_communication_flags & configs::proxy_to_acl_mask), "reference to a writable variable isn't safe");
        return static_cast<T&>(this->variables).value;
    }

    template<class T, class... Args>
    void set(Args && ... args)
    {
        static_assert(!(T::acl_proxy_communication_flags & configs::proxy_to_acl_mask),
            "T is writable, use set_acl<T>() instead.");
        this->set_value<T>(static_cast<Args&&>(args)...);
    }

    template<class T, class... Args>
    void set_acl(Args && ... args)
    {
        static_assert(T::acl_proxy_communication_flags & configs::proxy_to_acl_mask,
            "T isn't writable, use set<T>() instead.");
        this->set_value<T>(static_cast<Args&&>(args)...);
    }

    template<class T, class F>
    void update(F&& f)
    {
        static_assert(!(T::acl_proxy_communication_flags & configs::proxy_to_acl_mask),
            "T is writable, use update_acl<T>() instead.");
        this->update_value<T>(static_cast<F&&>(f));
    }

    template<class T, class F>
    void update_acl(F&& f)
    {
        static_assert(T::acl_proxy_communication_flags & configs::proxy_to_acl_mask,
            "T isn't writable, use update<T>() instead.");
        this->update_value<T>(static_cast<F&&>(f));
    }

    template<class T>
    void send()
    {
        static_assert(T::acl_proxy_communication_flags & configs::proxy_to_acl_mask, "T isn't writable.");
        this->to_send_index.insert(T::index);
        this->asked_table.clear(T::index);
    }

    template<class T>
    void ask() noexcept
    {
        static_assert(T::acl_proxy_communication_flags & configs::acl_to_proxy_mask, "T isn't askable");
        this->to_send_index.insert(T::index);
        this->asked_table.set(T::index);
    }

    template<class T>
    bool is_asked() const noexcept
    {
        static_assert(T::acl_proxy_communication_flags & configs::acl_to_proxy_mask, "T isn't askable");
        return this->asked_table.get(T::index);
    }

    template<class T>
    bool is_ready_to_send() const noexcept
    {
        static_assert(T::acl_proxy_communication_flags & configs::proxy_to_acl_mask, "T isn't writable.");
        return this->to_send_index.get(T::index);
    }

    struct ConfigurationHolder final : ::ConfigurationHolder
    {
        void set_section(zstring_view section) override;
        void set_value(zstring_view key, zstring_view value) override;

        explicit ConfigurationHolder(Inifile & ini) noexcept
        : variables(ini.variables)
        {}

        ConfigurationHolder& as_ref()
        {
            return *this;
        }

    private:
        int section_id = 0;
        char const* section_name = "";
        configs::VariablesConfiguration & variables;
    };

    void reset_ini_values();

    using ZStringBuffer = std::array<char, configs::max_str_buffer_size + 1>;

    using LoggableCategory = configs::LoggableCategory;

    struct FieldConstReference
    {
        [[nodiscard]] bool is_asked() const noexcept
        {
            return this->ini->asked_table.get(this->id);
        }

        [[nodiscard]] zstring_view to_zstring_view(ZStringBuffer& buffer) const;

        [[nodiscard]] zstring_view get_acl_name() const noexcept;

        [[nodiscard]] authid_t authid() const noexcept
        {
            return this->id;
        }

        [[nodiscard]] LoggableCategory loggable_category() const noexcept
        {
            return configs::loggable_field(safe_cast<unsigned>(this->authid()));
        }

    private:
        Inifile const* ini;
        authid_t id;

        FieldConstReference(Inifile const& ini, authid_t id) noexcept
        : ini(&ini)
        , id(id)
        {}

        friend class Inifile;
    };

    struct FieldReference
    {
        bool is_asked() const noexcept
        {
            assert(bool(*this));
            return FieldConstReference(*this->ini, this->id).is_asked();
        }

        zstring_view to_zstring_view(ZStringBuffer& buffer) const
        {
            assert(bool(*this));
            return FieldConstReference(*this->ini, this->id).to_zstring_view(buffer);
        }

        zstring_view get_acl_name() const noexcept
        {
            assert(bool(*this));
            return FieldConstReference(*this->ini, this->id).get_acl_name();
        }

        LoggableCategory loggable_category() const noexcept
        {
            assert(bool(*this));
            return configs::loggable_field(safe_cast<unsigned>(this->id));
        }

        void ask() noexcept
        {
            assert(bool(*this));
            this->ini->asked_table.set(this->id);
        }

        explicit operator bool () const noexcept
        {
            return this->id != configs::max_authid;
        }

        [[nodiscard]] authid_t authid() const noexcept
        {
            return this->id;
        }

        bool parse(bytes_view value);

    private:
        Inifile* ini = nullptr;
        authid_t id = configs::max_authid;

        FieldReference() = default;

        FieldReference(Inifile& ini, authid_t id) noexcept
        : ini(&ini)
        , id(id)
        {}

        friend class Inifile;
    };

    FieldReference get_acl_field_by_name(chars_view name) noexcept;

    void clear_acl_fields_changed() noexcept
    {
        this->to_send_index.clear();
    }

    struct FieldsChanged
    {
        struct iterator
        {
            iterator & operator++() noexcept
            {
                ++this->it;
                return *this;
            }

            bool operator != (iterator const & other) const noexcept
            {
                return this->it != other.it;
            }

            FieldConstReference operator*() const noexcept
            {
                return {*this->ini, *this->it};
            }

        private:
            authid_t const * it;
            Inifile const * ini;

            friend struct FieldsChanged;

            iterator(authid_t const * it, Inifile const & ini) noexcept
            : it(it)
            , ini(&ini)
            {}
        };

        iterator begin() const noexcept
        {
            return {this->ini->to_send_index.cbegin(), *this->ini};
        }

        iterator end() const noexcept
        {
            return {this->ini->to_send_index.cend(), *this->ini};
        }

        std::size_t size() const noexcept
        {
            return this->ini->to_send_index.size();
        }

    private:
        Inifile const * ini;

        friend class Inifile;

        constexpr FieldsChanged(Inifile const & ini) noexcept
        :ini(&ini)
        {}
    };

    FieldsChanged get_acl_fields_changed() const noexcept
    {
        return {*this};
    }

    void copy_variables_to(Inifile& ini) const
    {
        ini.variables = variables;
    }

private:
    struct AuthIdBoolTable
    {
        using uint_fast = uint_fast32_t;
        static_assert(sizeof(uint_fast) >= sizeof(authid_t));
        static const std::size_t count = configs::VariablesAclPack::size;
        static const std::size_t word_count
            = (count + sizeof(uint_fast) - 1) / sizeof(uint_fast);

        std::array<uint_fast, word_count> words {};

        void set(authid_t id) noexcept
        {
            uint_fast i = uint_fast(id);
            this->get_word(i) |= this->get_mask(i);
        }

        void clear(authid_t id) noexcept
        {
            uint_fast i = uint_fast(id);
            this->get_word(i) &= ~this->get_mask(i);
        }

        [[nodiscard]] bool get(authid_t id) const noexcept
        {
            uint_fast i = uint_fast(id);
            return bool(this->get_word(i) & this->get_mask(i));
        }

        [[nodiscard]] static uint_fast get_mask(uint_fast i) noexcept
        {
            return uint_fast{1} << (i % sizeof(uint_fast));
        }

        [[nodiscard]] uint_fast& get_word(uint_fast i) noexcept
        {
            return this->words[i / sizeof(uint_fast)];
        }

        [[nodiscard]] uint_fast get_word(uint_fast i) const noexcept
        {
            return this->words[i / sizeof(uint_fast)];
        }
    };

    class ToSendIndexList
    {
        AuthIdBoolTable bool_table;
        std::array<authid_t, configs::VariablesAclPack::size> list;
        std::size_t list_size = 0;

    public:
        ToSendIndexList() = default;

        void insert(authid_t id) noexcept
        {
            using uint_fast = AuthIdBoolTable::uint_fast;
            uint_fast i = uint_fast(id);
            uint_fast mask = this->bool_table.get_mask(i);
            uint_fast & word = this->bool_table.get_word(i);
            if (!(word & mask)) {
                word |= mask;
                this->list[this->list_size++] = id;
            }
        }

        [[nodiscard]] bool get(authid_t id) const noexcept
        {
            return this->bool_table.get(id);
        }

        void clear() noexcept
        {
            this->bool_table.words.fill(0);
            this->list_size = 0;
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return this->list_size;
        }

        [[nodiscard]] authid_t const * cbegin() const noexcept
        {
            return this->list.data();
        }

        [[nodiscard]] authid_t const * cend() const noexcept
        {
            return this->cbegin() + this->size();
        }
    };

    AuthIdBoolTable asked_table;
    ToSendIndexList to_send_index;
    configs::VariablesConfiguration variables;

    template<class T>
    void push_to_send_index() noexcept
    {
        static_assert(T::acl_proxy_communication_flags & configs::proxy_to_acl_mask, "is not writable");
        this->to_send_index.insert(T::index);
    }

    void initialize();

    template<class T, class Spec, class = void>
    struct set_value_impl
    {
        template<class U>
        static void impl(T & x, U && new_value)
        {
            x = static_cast<U&&>(new_value);
        }

        template<class U, class... Ts>
        static void impl(T & x, U && param1, Ts && ... other_params)
        {
            x = {static_cast<U&&>(param1), static_cast<Ts&&>(other_params)...};
        }
    };

    template<class T, class Void>
    struct set_value_impl<T, std::string, Void>
    {
        template<class U>
        static void impl(T & x, U && new_value)
        {
            using Decay = std::remove_cv_t<std::remove_reference_t<U>>;
            static_assert(!std::is_integral_v<Decay> || std::is_same_v<Decay, char>, "expected a string type or char, not an int");
            x = static_cast<U&&>(new_value);
        }

        template<class U, class... Ts>
        static void impl(T & x, U && param1, Ts && ... other_params)
        {
            x = {static_cast<U&&>(param1), static_cast<Ts&&>(other_params)...};
        }
    };

    template<class Void>
    struct set_value_impl<std::array<unsigned char, 32>, configs::spec_types::fixed_binary, Void>
    {
        static constexpr std::size_t N = 32;
        using T = std::array<unsigned char, N>;

        static void impl(T & x, T const & arr)
        {
            x = arr;
        }

        static void impl(T & x, unsigned char const (&arr)[N])
        {
            std::copy(arr+0, arr+N, x.begin());
        }

        static void impl(T & x, char const (&arr)[N+1])
        {
            assert(arr[N] == '\0');
            std::copy(arr+0, arr+N, x.begin());
        }

        // template<class U>
        // static void impl(T & x, U const * arr, std::size_t n)
        // {
        //     assert(N >= n);
        //     std::copy(arr, arr + n, begin(x));
        // }
    };

    template<std::size_t N, class Void>
    struct set_value_impl<char[N], char[N], Void>
    {
        using T = char[N];

        static void impl(T & x, char const * s, std::size_t n)
        {
            assert(N > n);
            n = std::min(n, N-1);
            memcpy(x, s, n);
            x[n] = 0;
        }

        static void impl(T & x, char const * s)
        {
            impl(x, s, strnlen(s, N-1));
        }

        static void impl(T & x, std::string const & str)
        {
            impl(x, str.data(), str.size());
        }
    };

    template<std::size_t N, class Void>
    struct set_value_impl<char[N], configs::spec_types::fixed_string, Void>
    {
        using T = char[N];

        static void impl(T & x, char const * s, std::size_t n)
        {
            assert(N >= n);
            n = std::min(n, N-1);
            memcpy(x, s, n);
            memset(x + n, 0, N - n);
        }

        static void impl(T & x, char const * s)
        {
            impl(x, s, strnlen(s, N-1));
        }

        static void impl(T & x, std::string const & str)
        {
            impl(x, str.data(), str.size());
        }
    };

    template<class T,
        configs::spec_types::underlying_type_for_range_t<T> min,
        configs::spec_types::underlying_type_for_range_t<T> max,
        class Void>
    struct set_value_impl<T, configs::spec_types::range<T, min, max>, Void>
    {
        static void impl(T & x, T new_value)
        {
            assert(T{min} <= new_value || new_value <= T{max});
            x = new_value;
        }
    };

    template<class T, class... Args>
    void set_value(Args && ... args)
    {
        auto& value = static_cast<T&>(this->variables).value;
        set_value_impl<std::remove_reference_t<decltype(value)>, typename T::mapped_type>
            ::impl(value, static_cast<Args&&>(args)...);
        this->update_state<T>();
    }

    template<class T, class F>
    void update_value(F&& f)
    {
        auto& value = static_cast<T&>(this->variables).value;
        static_cast<F&&>(f)(value);
        this->update_state<T>();
    }

    template<class T>
    void update_state()
    {
        if constexpr (T::acl_proxy_communication_flags & configs::proxy_to_acl_mask) {
            this->to_send_index.insert(T::index);
        }

        if constexpr (T::acl_proxy_communication_flags) {
            this->asked_table.clear(T::index);
        }
    }
};

