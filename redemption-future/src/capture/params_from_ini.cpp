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
*   Author(s): Christophe Grosjean
*/

#include "capture/params_from_ini.hpp"
#include "utils/sugar/numerics/safe_conversions.hpp"

#include "configs/config.hpp"


OcrParams ocr_params_from_ini(const Inifile & ini)
{
    return OcrParams{
        ini.get<cfg::ocr::version>(),
        ocr::locale::LocaleId(ini.get<cfg::ocr::locale>()),
        ini.get<cfg::ocr::on_title_bar_only>(),
        ini.get<cfg::ocr::max_unrecog_char_rate>(),
        ini.get<cfg::ocr::interval>(),
        ini.get<cfg::debug::ocr>()
    };
}

PatternParams pattern_params_from_ini(const Inifile & ini)
{
    return PatternParams{
        ini.get<cfg::context::pattern_notify>().c_str(),
        ini.get<cfg::context::pattern_kill>().c_str(),
        ini.get<cfg::debug::capture>()
    };
}

WrmParams wrm_params_from_ini(
    BitsPerPixel capture_bpp, bool remote_app, CryptoContext & cctx, Random & rnd,
    const char * hash_path, const Inifile & ini)
{
    return WrmParams{
        capture_bpp,
        remote_app,
        cctx,
        rnd,
        hash_path,
        ini.get<cfg::capture::wrm_break_interval>(),
        ini.get<cfg::capture::wrm_compression_algorithm>(),
        safe_cast<RDPSerializerVerbose>(ini.get<cfg::debug::capture>()),
    };
}

RedisParams redis_params_from_ini(const Inifile & ini)
{
    return RedisParams{
        .address = ini.get<cfg::audit::redis_address>(),
        .port = ini.get<cfg::audit::redis_port>(),
        .password = ini.get<cfg::audit::redis_password>(),
        .db = ini.get<cfg::audit::redis_db>(),
        .timeout = ini.get<cfg::audit::redis_timeout>(),
        .tls = RedisParams::Tls{
            .enable_tls = ini.get<cfg::audit::redis_use_tls>(),
            .cert_file = ini.get<cfg::audit::redis_tls_cert>().c_str(),
            .key_file = ini.get<cfg::audit::redis_tls_key>().c_str(),
            .ca_cert_file = ini.get<cfg::audit::redis_tls_cacert>().c_str(),
        }
    };
}
