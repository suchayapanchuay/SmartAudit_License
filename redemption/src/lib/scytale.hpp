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
   Author(s): Christophe Grosjean, Jonathan Poelen
*/

#pragma once

#include "cxx/cxx.hpp"
#include "capture/cryptofile.hpp"

extern "C"
{
    struct ScytaleWriterHandle;
    struct ScytaleReaderHandle;
    struct ScytaleMetaReaderHandle;
    struct ScytaleKeyHandle;

    struct ScytaleFdxWriterHandle;
    struct ScytaleTflWriterHandler;
    struct ScytaleMwrm3ReaderHandle;

    using HashHexArray = char[MD_HASH::DIGEST_LENGTH * 2 + 1];


    REDEMPTION_LIB_EXPORT
    char const * scytale_version();

    // Note: hmac_key is a 32 bytes array


    // Writer
    //@{
    REDEMPTION_LIB_EXPORT
    ScytaleWriterHandle * scytale_writer_new(
        int with_encryption, int with_checksum,
        uint8_t const * master_derivator, unsigned master_derivator_len,
        uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
        int old_scheme, int one_shot);

    REDEMPTION_LIB_EXPORT
    ScytaleWriterHandle * scytale_writer_new_with_test_random(
        int with_encryption, int with_checksum,
        uint8_t const * master_derivator, unsigned master_derivator_len,
        uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
        int old_scheme, int one_shot);

    REDEMPTION_LIB_EXPORT
    char const * scytale_writer_get_error_message(ScytaleWriterHandle * handle);

    // When derivator is nullptr, computed from record_path
    // When permissions is 0, auto-transformed to 0440
    REDEMPTION_LIB_EXPORT
    int scytale_writer_open(
        ScytaleWriterHandle * handle,
        char const * record_path, char const * hash_path,
        int permissions,
        uint8_t const * derivator, unsigned derivator_len);

    REDEMPTION_LIB_EXPORT
    int scytale_writer_write(
        ScytaleWriterHandle * handle, uint8_t const * buffer, unsigned long len);

    /// \return HashHexArray
    REDEMPTION_LIB_EXPORT
    char const * scytale_writer_get_qhashhex(ScytaleWriterHandle * handle);

    /// \return HashHexArray
    REDEMPTION_LIB_EXPORT
    char const * scytale_writer_get_fhashhex(ScytaleWriterHandle * handle);

    REDEMPTION_LIB_EXPORT
    int scytale_writer_close(ScytaleWriterHandle * handle);

    REDEMPTION_LIB_EXPORT
    void scytale_writer_delete(ScytaleWriterHandle * handle);
    //@}


    // Reader
    //@{
    REDEMPTION_LIB_EXPORT
    ScytaleReaderHandle * scytale_reader_new(
        uint8_t const * master_derivator, unsigned master_derivator_len,
        uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
        int old_scheme, int one_shot);

    REDEMPTION_LIB_EXPORT
    char const * scytale_reader_get_error_message(ScytaleReaderHandle * handle);

    /// When derivator is nullptr, computed from record_path
    REDEMPTION_LIB_EXPORT
    int scytale_reader_open(
        ScytaleReaderHandle * handle, char const * path,
        uint8_t const * derivator, unsigned derivator_len);

    // enum class EncryptionSchemeTypeResult
    // {
    //     Error = -1,
    //     NoEncrypted = 0,
    //     OldScheme,
    //     NewScheme,
    // };
    // When derivator is nullptr, computed from record_path
    /// \result EncryptionSchemeTypeResult
    REDEMPTION_LIB_EXPORT
    int scytale_reader_open_with_auto_detect_encryption_scheme(
        ScytaleReaderHandle * handle, char const * path,
        uint8_t const * derivator, unsigned derivator_len);

    /// < 0: error, 0: eof, >0: length read
    REDEMPTION_LIB_EXPORT
    long long scytale_reader_read(ScytaleReaderHandle * handle,
        uint8_t * buffer, unsigned long len);

    /// Copy content in a fd.
    REDEMPTION_LIB_EXPORT
    int scytale_reader_send_to(ScytaleReaderHandle * handle, int fd_out);

    REDEMPTION_LIB_EXPORT
    int scytale_reader_close(ScytaleReaderHandle * handle);

    REDEMPTION_LIB_EXPORT
    void scytale_reader_delete(ScytaleReaderHandle * handle);

    REDEMPTION_LIB_EXPORT
    int scytale_reader_fhash(ScytaleReaderHandle * handle, const char * filename);

    REDEMPTION_LIB_EXPORT
    int scytale_reader_qhash(ScytaleReaderHandle * handle, const char * filename);

    /// \return HashHexArray
    REDEMPTION_LIB_EXPORT
    const char * scytale_reader_get_qhashhex(ScytaleReaderHandle * handle);

    /// \return HashHexArray
    REDEMPTION_LIB_EXPORT
    const char * scytale_reader_get_fhashhex(ScytaleReaderHandle * handle);
    //@}


    // Meta reader
    //@{
    REDEMPTION_LIB_EXPORT
    ScytaleMetaReaderHandle * scytale_meta_reader_new(ScytaleReaderHandle * reader);

    REDEMPTION_LIB_EXPORT
    char const * scytale_meta_reader_get_error_message(ScytaleMetaReaderHandle * handle);

    REDEMPTION_LIB_EXPORT
    int scytale_meta_reader_read_hash(
        ScytaleMetaReaderHandle * handle, int version, int has_checksum);

    REDEMPTION_LIB_EXPORT
    int scytale_meta_reader_read_header(ScytaleMetaReaderHandle * handle);

    REDEMPTION_LIB_EXPORT
    int scytale_meta_reader_read_line(ScytaleMetaReaderHandle * handle);

    REDEMPTION_LIB_EXPORT
    int scytale_meta_reader_read_line_eof(ScytaleMetaReaderHandle * handle);

    REDEMPTION_LIB_EXPORT
    void scytale_meta_reader_delete(ScytaleMetaReaderHandle * handle);

    struct ScytaleMwrmHeader
    {
        int version;
        int has_checksum;
    };

    struct ScytaleMwrmLine
    {
        char const * filename;
        uint64_t size;
        uint64_t mode;
        uint64_t uid;
        uint64_t gid;
        uint64_t dev;
        uint64_t ino;
        uint64_t mtime;
        uint64_t ctime;
        uint64_t start_time;
        uint64_t stop_time;
        int with_hash;
        char const * qhash;
        char const * fhash;
    };

    REDEMPTION_LIB_EXPORT
    ScytaleMwrmHeader * scytale_meta_reader_get_header(ScytaleMetaReaderHandle * handle);

    REDEMPTION_LIB_EXPORT
    ScytaleMwrmLine * scytale_meta_reader_get_line(ScytaleMetaReaderHandle * handle);
    //@}


    // Fdx / Tfl
    //@{
    // When permissions is 0, auto-transformed to 0440
    REDEMPTION_LIB_EXPORT
    ScytaleFdxWriterHandle * scytale_fdx_writer_new(
        int with_encryption, int with_checksum,
        uint8_t const * master_derivator, unsigned master_derivator_len,
        uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
        char const * record_path, char const * hash_path, char const * fdx_file_base,
        char const * sid, int permissions);

    // When permissions is 0, auto-transformed to 0440
    REDEMPTION_LIB_EXPORT
    ScytaleFdxWriterHandle * scytale_fdx_writer_new_with_test_random(
        int with_encryption, int with_checksum,
        uint8_t const * master_derivator, unsigned master_derivator_len,
        uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
        char const * record_path, char const * hash_path, char const * fdx_file_base,
        char const * sid, int permissions);

    REDEMPTION_LIB_EXPORT
    char const * scytale_fdx_get_path(ScytaleFdxWriterHandle * handle);

    /// \param direction  Mwrm3::Direction
    REDEMPTION_LIB_EXPORT
    ScytaleTflWriterHandler * scytale_fdx_writer_open_tfl(
        ScytaleFdxWriterHandle * handle, char const * filename, int direction);

    REDEMPTION_LIB_EXPORT
    int scytale_tfl_writer_write(
        ScytaleTflWriterHandler * handle, uint8_t const * buffer, unsigned long len);

    /// len should be 32
    /// \param transfered_status  Mwrm3::TransferedStatus
    REDEMPTION_LIB_EXPORT
    int scytale_tfl_writer_close(
        ScytaleTflWriterHandler * handle, int transfered_status,
        uint8_t const* sig, unsigned long len);

    REDEMPTION_LIB_EXPORT
    int scytale_tfl_writer_cancel(ScytaleTflWriterHandler * handle);

    /// \return HashHexArray
    REDEMPTION_LIB_EXPORT
    char const * scytale_fdx_writer_get_qhashhex(ScytaleFdxWriterHandle * handle);

    /// \return HashHexArray
    REDEMPTION_LIB_EXPORT
    char const * scytale_fdx_writer_get_fhashhex(ScytaleFdxWriterHandle * handle);

    /// \return 1 if not open, 0 is ok, -1 if error
    REDEMPTION_LIB_EXPORT
    int scytale_fdx_writer_close(ScytaleFdxWriterHandle * handle);

    REDEMPTION_LIB_EXPORT
    int scytale_fdx_writer_delete(ScytaleFdxWriterHandle * handle);

    REDEMPTION_LIB_EXPORT
    char const * scytale_fdx_writer_get_error_message(
        ScytaleFdxWriterHandle * handle);
    //@}

    // Mwrm3 Reader
    //@{
    REDEMPTION_LIB_EXPORT
    ScytaleMwrm3ReaderHandle * scytale_mwrm3_reader_new(ScytaleReaderHandle * reader);

    struct ScytaleMwrm3ReaderData
    {
        uint32_t type;
        // 'u': uint64_t  'i': int64_t  's': {char*, uint32_t}  'B': {uint8_t*, uint32_t}
        char const* fmt;
        void const* data;
    };

    // return nullptr if error or truncated file (truncated if reader is No Error + eof())
    REDEMPTION_LIB_EXPORT
    ScytaleMwrm3ReaderData const* scytale_mwrm3_reader_read_next(
        ScytaleMwrm3ReaderHandle * handle);

    REDEMPTION_LIB_EXPORT
    char const * scytale_mwrm3_reader_get_error_message(ScytaleMwrm3ReaderHandle * handle);

    REDEMPTION_LIB_EXPORT
    int scytale_mwrm3_reader_delete(ScytaleMwrm3ReaderHandle * handle);
    //@}
}
