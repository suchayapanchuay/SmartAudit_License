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
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Meng Tan
*/

#pragma once

#include "core/error.hpp"
#include "system/basic_hmac.hpp"
#include "utils/sugar/bounded_array_view.hpp"

#include <cstdint>
#include <cstring>
#include <cassert>

#include <openssl/sha.h>

# if OPENSSL_VERSION_NUMBER >= 0x30000000L

#include "utils/sugar/movable_ptr.hpp"

#include <openssl/evp.h>

class SslSha256
{
    movable_ptr<EVP_MD_CTX> sha256;

public:
    enum : unsigned { DIGEST_LENGTH = SHA256_DIGEST_LENGTH };

    SslSha256()
    {
        reset();
    }

    SslSha256(SslSha256 &&) = default;
    SslSha256& operator=(SslSha256 &&) = default;

    ~SslSha256()
    {
        EVP_MD_CTX_free(sha256);
    }

    void update(bytes_view data)
    {
        if (EVP_DigestUpdate(sha256, data.as_u8p(), data.size()) != 1) {
            throw Error(ERR_SSL_CALL_SHA256_UPDATE_FAILED);
        }
    }

    void final(sized_writable_u8_array_view<DIGEST_LENGTH> out_data)
    {
        unsigned int out_len = DIGEST_LENGTH;
        if (EVP_DigestFinal_ex(sha256, out_data.data(), &out_len) != 1) {
            throw Error(ERR_SSL_CALL_SHA256_FINAL_FAILED);
        }
        assert(out_len == DIGEST_LENGTH);
    }

    void reset()
    {
        if (sha256) {
            EVP_MD_CTX_free(sha256);
        }
        sha256 = EVP_MD_CTX_new();
        if (!sha256) {
            throw Error(ERR_SSL_CALL_SHA256_INIT_FAILED);
        }

        const EVP_MD* sha256_md = EVP_sha256();

        if (EVP_DigestInit_ex(sha256, sha256_md, nullptr) != 1) {
            throw Error(ERR_SSL_CALL_SHA256_INIT_FAILED);
        }
    }
};

# else

class SslSha256
{
    SHA256_CTX sha256;

public:
    enum : unsigned { DIGEST_LENGTH = SHA256_DIGEST_LENGTH };

    SslSha256()
    {
        reset();
    }

    void update(bytes_view data)
    {
        if (0 == SHA256_Update(&this->sha256, data.as_u8p(), data.size())){
            throw Error(ERR_SSL_CALL_SHA256_UPDATE_FAILED);
        }
    }

    void final(sized_writable_u8_array_view<DIGEST_LENGTH> out_data)
    {
        if (0 == SHA256_Final(out_data.data(), &this->sha256)){
            throw Error(ERR_SSL_CALL_SHA256_FINAL_FAILED);
        }
    }

    void reset()
    {
        if (0 == SHA256_Init(&this->sha256)){
            throw Error(ERR_SSL_CALL_SHA256_INIT_FAILED);
        }
    }
};

# endif

using SslHMAC_Sha256 = detail_::basic_HMAC<&EVP_sha256, SslSha256::DIGEST_LENGTH>;
using SslHMAC_Sha256_Delayed = detail_::DelayedHMAC<&EVP_sha256, SslSha256::DIGEST_LENGTH>;
