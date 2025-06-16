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
   Copyright (C) Wallix 2017
   Author(s): Christophe Grosjean, Raphael ZHOU
*/

#pragma once

#include "utils/sugar/split.hpp"
#include "utils/strutils.hpp"
#include "utils/ascii.hpp"
#include "acl/auth_api.hpp"

#include <type_traits>
#include <string_view>


struct ParseServerMessage
{
    static const std::size_t max_arity = 64;
    static const std::size_t max_order_len = 126;

    TaggedStringArray<UpperTag, max_order_len> upper_order() const noexcept
    {
        return ascii_to_limited_upper<max_order_len>(order_);
    }

    array_view<std::string_view> parameters() const noexcept
    {
        return array_view(parameters_).first(parameters_size_);
    }

    bool parse(chars_view msg)
    {
        order_ = {};
        parameters_size_ = {};

        if (const char * separator = strchr(msg, '=')) {
            order_ = msg.before(separator);

            for (auto param : split_with(msg.after(separator), '\x01')) {
                if (parameters_size_ == max_arity) {
                    return false;
                }
                parameters_[parameters_size_] = param.as<std::string_view>();
                ++parameters_size_;
            }
        }
        else {
            order_ = msg;
        }

        return true;
    }

private:
    chars_view order_ {};
    std::size_t parameters_size_ {};
    std::array<std::string_view, max_arity> parameters_;
};

template<std::size_t N>
struct ExecutableLog6IfCtx
{
    static constexpr std::size_t nb_key = N;

    TaggedStringView<UpperTag> order;
    LogId logid;
    std::array<chars_view, N> keys;
};

template<class... AV>
ExecutableLog6IfCtx<sizeof...(AV)>
executable_log6_if(TaggedStringView<UpperTag> order, LogId logid, AV... av) noexcept
{
    return {order, logid, {av...}};
}

#define EXECUTABLE_LOG6_ID_AND_NAME(name) #name ""_ascii_upper, LogId::name

template<class Fn, class... Executables>
bool execute_log6_if(
    TaggedStringView<UpperTag> order, array_view<std::string_view> parameters,
    Fn&& fn, Executables&&... executables)
{
    auto exec = [&](auto& executable){
        constexpr std::size_t nb_key = std::remove_reference_t<decltype(executable)>::nb_key;
        if (nb_key != parameters.size() || order != executable.order) {
            return false;
        }
        std::array<KVLog, nb_key> kvlogs;
        for (std::size_t i = 0; i < nb_key; ++i) {
            kvlogs[i].key = executable.keys[i];
            kvlogs[i].value = parameters[i];
        }
        fn(executable.logid, KVLogList{kvlogs});
        return true;
    };
    return (... || exec(executables));
}
