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
   Author(s): Christophe Grosjean, Jonatan Poelen
*/

#pragma once

#include "transport/crypto_transport.hpp"
#include "utils/real_clock.hpp"

class AclReportApi;

struct OutMetaSequenceTransport final : SequencedTransport
{
    OutMetaSequenceTransport(
        CryptoContext & cctx,
        Random & rnd,
        const char * path,
        const char * hash_path,
        const char * basename,
        RealTimePoint now,
        uint16_t width,
        uint16_t height,
        AclReportApi * acl_report,
        FilePermissions file_permissions);

    ~OutMetaSequenceTransport();

    void timestamp(RealTimePoint tp);

    bool next() override;

    bool disconnect() override;

private:
    void do_send(const uint8_t * data, size_t len) override;

    void do_close();

    void next_meta_file();

    class WrmFGen
    {
        char         path[1024];
        char         hash_path[1024];
        char         filename[1012];
        char         extension[12];
        mutable char filename_gen[2070];
        mutable char hash_filename_gen[2070];

    public:
        WrmFGen(
            const char * const prefix,
            const char * const hash_prefix,
            const char * const filename,
            const char * const extension);

        const char * get_filename(unsigned count) const noexcept;

        const char * get_hash_filename(unsigned count) const noexcept;

    };

    struct MetaFilename
    {
        char filename[2048];
        MetaFilename(const char * path, const char * basename);
    };

    OutCryptoTransport meta_buf_encrypt_transport;
    OutCryptoTransport wrm_filter_encrypt_transport;

    char current_filename_[1024] {};
    WrmFGen filegen_;
    unsigned num_file_ = 0;

    MetaFilename mf_;
    MetaFilename hf_;
    std::chrono::seconds start_sec_;
    std::chrono::seconds stop_sec_;

    CryptoContext & cctx;

    FilePermissions file_permissions_;
};
