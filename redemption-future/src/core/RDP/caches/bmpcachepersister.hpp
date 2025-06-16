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
    Author(s): Christophe Grosjean, Raphael Zhou
*/


#pragma once

#include "core/RDP/caches/bmpcache.hpp"
#include "transport/file_transport.hpp"
#include "utils/verbose_flags.hpp"
#include "utils/fileutils.hpp"
#include "utils/sugar/unique_fd.hpp"
#include "utils/sugar/int_to_chars.hpp"
#include "utils/basic_function.hpp"

#include <unordered_map>


namespace RDP {
struct BitmapCachePersistentListEntry;
} // namespace RDP

class BmpCachePersister
{
private:
    static const uint8_t CURRENT_VERSION = 1;

    using map_value = Bitmap;
    using map_key = uint64_t;

    static map_key tokey(uint8_t const (&sig)[8]) noexcept
    {
        return (map_key(sig[0]) << 56)
             | (map_key(sig[1]) << 48)
             | (map_key(sig[2]) << 40)
             | (map_key(sig[3]) << 32)
             | (map_key(sig[4]) << 24)
             | (map_key(sig[5]) << 16)
             | (map_key(sig[6]) << 8)
             | (map_key(sig[7]) << 0)
        ;
    }

    struct KeyString
    {
        explicit KeyString(map_key sig) noexcept
        {
            *int_to_fixed_hexadecimal_upper_chars(this->s, sig) = '\0';
        }

        [[nodiscard]] const char * c_str() const noexcept
        { return this->s; }

    private:
        char s[17];
    };

    using container_type = std::unordered_map<map_key, map_value>;

    container_type bmp_map[BmpCache::MAXIMUM_NUMBER_OF_CACHES];

    BmpCache & bmp_cache;

public:
    REDEMPTION_VERBOSE_FLAGS(private, verbose)
    {
        none,
        from_disk = 0x00'0001,
        bmp_info  = 0x10'0000
    };

    // Preloads bitmap from file to be used later with Client Persistent Key List PDUs.
    BmpCachePersister(BmpCache & bmp_cache, Transport & t, const char * filename, Verbose verbose)
    : bmp_cache(bmp_cache)
    , verbose(verbose)
    {
        uint8_t buf[16];
        InStream stream(buf);

        t.recv_boom(buf, 5);  /* magic(4) + version(1) */

        auto magic      = stream.in_skip_bytes(4);  /* magic(4) */
        uint8_t version = stream.in_uint8();

        //LOG( LOG_INFO, "BmpCachePersister: magic=\"%c%c%c%c\""
        //   , magic[0], magic[1], magic[2], magic[3]);
        //LOG( LOG_INFO, "BmpCachePersister: version=%u", version);

        if (0 != ::memcmp(magic.data(), "PDBC", magic.size())) {
            LOG( LOG_ERR
               , "BmpCachePersister::BmpCachePersister: "
                 "File is not a persistent bitmap cache file. filename=\"%s\""
               , filename);
            throw Error(ERR_PDBC_LOAD);
        }

        if (version != CURRENT_VERSION) {
            LOG( LOG_ERR
               , "BmpCachePersister::BmpCachePersister: "
                 "Unsupported persistent bitmap cache file version(%u). filename=\"%s\""
               , version, filename);
            throw Error(ERR_PDBC_LOAD);
        }

        for (uint8_t cache_id = 0; cache_id < this->bmp_cache.number_of_cache; cache_id++) {
            this->preload_from_disk(t, cache_id);
        }
    }

private:
    static ScreenInfo extract_screen_info(InStream& stream)
    {
        BitsPerPixel original_bpp {stream.in_uint8()};
        assert(original_bpp == BitsPerPixel{8}
            || original_bpp == BitsPerPixel{15}
            || original_bpp == BitsPerPixel{16}
            || original_bpp == BitsPerPixel{24}
            || original_bpp == BitsPerPixel{32});

        uint16_t cx = stream.in_uint16_le();
        uint16_t cy = stream.in_uint16_le();

        return ScreenInfo{cx, cy, original_bpp};
    }

    void preload_from_disk(Transport & t, uint8_t cache_id) {
        uint8_t buf[65536];
        InStream stream(buf);
        auto end = buf;
        t.recv_boom(end, 2);
        end += 2;

        uint16_t bitmap_count = stream.in_uint16_le();
        LOG_IF(bool(this->verbose & Verbose::from_disk), LOG_INFO,
            "BmpCachePersister::preload_from_disk: bitmap_count=%u", bitmap_count);

        BGRPalette original_palette = BGRPalette::classic_332();

        for (uint16_t i = 0; i < bitmap_count; i++) {
            t.recv_boom(end, 13); // sig(8) + original_bpp(1) + cx(2) + cy(2);
            end += 13;

            uint8_t sig[8];
            stream.in_copy_bytes(sig, 8);

            auto original_info = extract_screen_info(stream);

            if (original_info.bpp == BitsPerPixel{8}) {
                // TODO implementation and endianness dependent
                t.recv_boom(end, sizeof(original_palette));
                end += sizeof(original_palette);

                stream.in_copy_bytes(const_cast<char*>(original_palette.data()), sizeof(original_palette)); /*NOLINT*/
            }

            uint16_t bmp_size;
            t.recv_boom(end, sizeof(bmp_size));
            bmp_size = stream.in_uint16_le();

            end = buf;
            stream = InStream(buf);

            t.recv_boom(end, bmp_size);

            if (bmp_cache.get_cache(cache_id).persistent()) {
                map_key key{tokey(sig)};

                LOG_IF(bool(this->verbose & Verbose::bmp_info), LOG_INFO,
                    "BmpCachePersister::preload_from_disk: sig=\"%s\" original_bpp=%u cx=%u cy=%u bmp_size=%u",
                    KeyString(key).c_str(), original_info.bpp,
                    original_info.width, original_info.height, bmp_size);

                assert(this->bmp_map[cache_id][key].is_valid() == false);

                Bitmap bmp( this->bmp_cache.bpp, original_info.bpp
                          , &original_palette
                          , safe_int(original_info.width), safe_int(original_info.height)
                          , stream.get_data(), bmp_size);

                uint8_t sha1[SslSha1::DIGEST_LENGTH];
                bmp.compute_sha1(sha1);
                if (0 != memcmp(sig, sha1, sizeof(sig))) {
                    LOG( LOG_ERR
                       , "BmpCachePersister::preload_from_disk: Preload failed. Cause: bitmap or key corruption.");
                    assert(false);
                }
                else {
                    this->bmp_map[cache_id][key] = bmp;
                }
            }

            end = buf;
            stream = InStream(buf);
        }
    }

public:
    // Places bitmaps of Persistent Key List into the cache.
    void process_key_list( uint8_t cache_id, RDP::BitmapCachePersistentListEntry * entries
                         , uint8_t number_of_entries, uint16_t first_entry_index) {
        uint16_t   max_number_of_entries = this->bmp_cache.get_cache(cache_id).size();
        uint16_t   cache_index           = first_entry_index;
        const union Sig {
            uint8_t  sig_8[8];
            uint32_t sig_32[2];
        }              * sig             = reinterpret_cast<const Sig *>(entries); /*NOLINT*/
        for (uint8_t entry_index = 0;
             (entry_index < number_of_entries) && (cache_index < max_number_of_entries);
             entry_index++, cache_index++, sig++) {
            assert(!this->bmp_cache.get_cache(cache_id)[cache_index]);

            map_key key{tokey(sig->sig_8)};

            container_type::iterator it = this->bmp_map[cache_id].find(key);
            if (it != this->bmp_map[cache_id].end()) {
                LOG_IF(bool(this->verbose & Verbose::bmp_info), LOG_INFO,
                    "BmpCachePersister: bitmap found. key=\"%s\"", KeyString(key).c_str());

                if (this->bmp_cache.get_cache(cache_id).size() > cache_index) {
                    this->bmp_cache.put(cache_id, cache_index, it->second, sig->sig_32[0], sig->sig_32[1]);
                }

                this->bmp_map[cache_id].erase(it);
            }
            else {
                LOG_IF(bool(this->verbose & Verbose::bmp_info),
                    LOG_WARNING, "BmpCachePersister: bitmap not found!!! key=\"%s\"", KeyString(key).c_str());
            }
        }
    }

    // Loads bitmap from file to be placed immediately into the cache.
    static void load_all_from_disk( BmpCache & bmp_cache, Transport & t, const char * filename
                                  , Verbose verbose) {
        uint8_t buf[16];
        InStream stream(buf);

        t.recv_boom(buf, 5);  /* magic(4) + version(1) */

        auto magic      = stream.in_skip_bytes(4);  /* magic(4) */
        uint8_t version = stream.in_uint8();

        //LOG( LOG_INFO, "BmpCachePersister: magic=\"%c%c%c%c\""
        //   , magic[0], magic[1], magic[2], magic[3]);
        //LOG( LOG_INFO, "BmpCachePersister: version=%u", version);

        if (0 != ::memcmp(magic.data(), "PDBC", magic.size())) {
            LOG( LOG_ERR
               , "BmpCachePersister::load_all_from_disk: "
                 "File is not a persistent bitmap cache file. filename=\"%s\""
               , filename);
            throw Error(ERR_PDBC_LOAD);
        }

        if (version != CURRENT_VERSION) {
            LOG( LOG_ERR
               , "BmpCachePersister::load_all_from_disk: "
                 "Unsupported persistent bitmap cache file version(%u). filename=\"%s\""
               , version, filename);
            throw Error(ERR_PDBC_LOAD);
        }

        for (uint8_t cache_id = 0; cache_id < bmp_cache.number_of_cache; cache_id++) {
            load_from_disk(bmp_cache, t, cache_id, verbose);
        }
    }

private:
    static void load_from_disk( BmpCache & bmp_cache, Transport & t
                              , uint8_t cache_id, Verbose verbose) {
        uint8_t buf[65536];
        InStream stream(buf);
        auto end = buf;
        t.recv_boom(end, 2);
        end += 2;

        uint16_t bitmap_count = stream.in_uint16_le();
        LOG_IF(bool(verbose & Verbose::from_disk), LOG_INFO,
            "BmpCachePersister::load_from_disk: bitmap_count=%u", bitmap_count);

        BGRPalette original_palette = BGRPalette::classic_332();

        for (uint16_t i = 0; i < bitmap_count; i++) {
            t.recv_boom(end, 13); // sig(8) + original_bpp(1) + cx(2) + cy(2);
            end +=  13;

            union {
                uint8_t  sig_8[8];
                uint32_t sig_32[2];
            } sig;

            stream.in_copy_bytes(sig.sig_8, 8); // sig(8);

            auto original_info = extract_screen_info(stream);

            if (original_info.bpp == BitsPerPixel{8}) {
                // TODO implementation and endianness dependent
                t.recv_boom(end, sizeof(original_palette));
                end += sizeof(original_palette);

                stream.in_copy_bytes(const_cast<char*>(original_palette.data()), sizeof(original_palette)); /*NOLINT*/
            }

            uint16_t bmp_size;
            t.recv_boom(end, sizeof(bmp_size));
            bmp_size = stream.in_uint16_le();

            end = buf;
            stream = InStream(buf);

            t.recv_boom(end, bmp_size);
            end += bmp_size;

            if (bmp_cache.get_cache(cache_id).persistent() && (i < bmp_cache.get_cache(cache_id).size())) {
                LOG_IF(bool(verbose & Verbose::bmp_info), LOG_INFO
                    , "BmpCachePersister::load_from_disk: sig=\"%s\" original_bpp=%u cx=%u cy=%u bmp_size=%u"
                    , KeyString(tokey(sig.sig_8)).c_str(), original_info.bpp
                    , original_info.width, original_info.height, bmp_size);


                Bitmap bmp( bmp_cache.bpp, original_info.bpp
                          , &original_palette
                          , safe_int(original_info.width), safe_int(original_info.height)
                          , stream.get_data(), stream.get_data() - end);

                bmp_cache.put(cache_id, i, bmp, sig.sig_32[0], sig.sig_32[1]);
            }

            end = buf;
            stream = InStream(buf);
        }
    }

public:
    // Saves content of cache to file.
    static void save_all_to_disk(const BmpCache & bmp_cache, Transport & t, Verbose verbose) {
        if (bool(verbose & Verbose::from_disk)) {
            bmp_cache.log();
        }

        StaticOutStream<128> stream;

        stream.out_copy_bytes("PDBC"_av);  // Magic(4)
        stream.out_uint8(CURRENT_VERSION);

        t.send(stream.get_produced_bytes());

        for (uint8_t cache_id = 0; cache_id < bmp_cache.number_of_cache; cache_id++) {
            save_to_disk(bmp_cache, cache_id, t, verbose);
        }
    }

private:
    static void save_to_disk(const BmpCache & bmp_cache, uint8_t cache_id, Transport & t, Verbose verbose) {
        uint16_t bitmap_count = 0;
        BmpCache::cache_ const & cache = bmp_cache.get_cache(cache_id);

        if (cache.persistent()) {
            for (auto const& cache_elem : cache.view()) {
                if (cache_elem) {
                    bitmap_count++;
                }
            }
        }

        StaticOutStream<65535> stream;
        stream.out_uint16_le(bitmap_count);
        t.send(stream.get_produced_bytes());
        if (!bitmap_count) {
            return;
        }

        for (auto const& cache_elem : cache.view()) {
            if (cache_elem) {
                stream.rewind();

                const Bitmap &   bmp      = cache_elem.bmp;
                const uint8_t (& sig)[8]  = cache_elem.sig.sig_8;
                const uint16_t   bmp_size = bmp.bmp_size();
                const uint8_t  * bmp_data = bmp.data();

                LOG_IF(bool(verbose & Verbose::bmp_info), LOG_INFO
                  , "BmpCachePersister::save_to_disk: sig=\"%s\" original_bpp=%u cx=%u cy=%u bmp_size=%u"
                  , KeyString(tokey(sig)).c_str(), bmp.bpp(), bmp.cx(), bmp.cy(), bmp_size);

                stream.out_copy_bytes(sig, 8);
                stream.out_uint8(safe_int(bmp.bpp()));
                stream.out_uint16_le(bmp.cx());
                stream.out_uint16_le(bmp.cy());
                if (bmp.bpp() == BitsPerPixel{8}) {
                    // TODO implementation and endianness dependent
                    stream.out_copy_bytes(bmp.palette().data(), sizeof(bmp.palette()));
                }
                stream.out_uint16_le(bmp_size);
                t.send(stream.get_produced_bytes());

                t.send(bmp_data, bmp_size);
            }
        }
    }
};

inline void save_persistent_disk_bitmap_cache(
    BmpCache const & bmp_cache,
    const char * persistent_path,
    const char * target_host,
    BitsPerPixel bpp,
    FileTransport::ErrorNotifier notify_error,
    BmpCachePersister::Verbose verbose
)
{
    // Ensures that the directory exists.
    if (::recursive_create_directory(persistent_path, S_IRWXU | S_IRWXG) != 0) {
        LOG( LOG_ERR
            , "save_persistent_disk_bitmap_cache: failed to create directory \"%s\"."
            , persistent_path);
        throw Error(ERR_BITMAP_CACHE_PERSISTENT, 0);
    }

    char filename_temporary[2048];
    ::snprintf(filename_temporary, sizeof(filename_temporary) - 1, "%s/PDBC-%s-%d-XXXXXX.tmp",
        persistent_path, target_host, underlying_cast(bpp));
    filename_temporary[sizeof(filename_temporary) - 1] = '\0';

    int fd = ::mkostemps(filename_temporary, 4, O_CREAT | O_WRONLY);
    if (fd == -1) {
        LOG( LOG_ERR
            , "save_persistent_disk_bitmap_cache: "
                "failed to open (temporary) file for writing. filename=\"%s\""
            , filename_temporary);
        throw Error(ERR_PDBC_SAVE);
    }

    try
    {
        {
            OutFileTransport oft(unique_fd{fd}, std::move(notify_error));
            BmpCachePersister::save_all_to_disk(bmp_cache, oft, verbose);
        }

        // Generates the name of file.
        char filename[2048];
        ::snprintf(filename, sizeof(filename) - 1, "%s/PDBC-%s-%d", persistent_path, target_host, underlying_cast(bpp));
        filename[sizeof(filename) - 1] = '\0';

        if (::rename(filename_temporary, filename) == -1) {
            LOG( LOG_WARNING
                , "save_persistent_disk_bitmap_cache: failed to rename the (temporary) file. "
                    "old_filename=\"%s\" new_filename=\"%s\""
                , filename_temporary, filename);
            ::unlink(filename_temporary);
        }
    }
    catch (...) {
        LOG( LOG_WARNING
            , "save_persistent_disk_bitmap_cache: failed to write (temporary) file. "
                "filename=\"%s\""
            , filename_temporary);
        ::unlink(filename_temporary);
    }
}
