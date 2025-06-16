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

#include "lib/scytale.hpp"
#include "transport/crypto_transport.hpp"
#include "transport/mwrm_reader.hpp"
#include "capture/mwrm3.hpp"
#include "capture/fdx_capture.hpp"
#include "test_only/lcg_random.hpp"
#include "system/urandom.hpp"
#include "utils/fileutils.hpp"
#include "utils/c_interface.hpp"
#include "utils/sugar/int_to_chars.hpp"
#include "utils/strutils.hpp"
#include "utils/string_c.hpp"
#include "utils/move_remaining_data.hpp"

#include "main/version.hpp"

#include <type_traits>
#include <memory>

#include <sys/sendfile.h> // sendfile


#define CHECK_NOTHROW(expr, errid) CHECK_NOTHROW_R(expr, -1, handle->error_ctx, errid)

#define CREATE_HANDLE(construct) [&]()->decltype(new construct){ \
    CHECK_NOTHROW_R(                                             \
        auto handle = new (std::nothrow) construct; /*NOLINT*/   \
        return handle,                                           \
        nullptr,                                                 \
        ScytaleErrorContext(),                                   \
        ERR_MEMORY_ALLOCATION_FAILED                             \
    );                                                           \
}()


using HashArray = uint8_t[MD_HASH::DIGEST_LENGTH];
static_assert(sizeof(HashArray) * 2 + 1 == sizeof(HashHexArray));

namespace
{
    void hash_to_hashhex(HashArray const & hash, HashHexArray& hashhex) noexcept {
        static_assert(sizeof(hash) * 2 + 1 == sizeof(HashHexArray));
        auto phex = hashhex;
        for (uint8_t c : hash) {
            phex = int_to_fixed_hexadecimal_upper_chars(phex, c);
        }
        *phex = '\0';
    }

    bytes_view compute_derivator(char const * path, uint8_t const * derivator, unsigned derivator_len) noexcept
    {
        if (derivator) {
            return {derivator, derivator_len};
        }
        size_t base_len = 0;
        const char * base = basename_len(path, base_len);
        return {base, base_len};
    }

    FilePermissions compute_permissions(int permissions) noexcept
    {
        return permissions
            ? FilePermissions(checked_int(permissions))
            : FilePermissions(0440);
    }

    struct HashHex
    {
        HashHex() noexcept
        {
            memset(this->hashhex, '0', sizeof(this->hashhex)-1); // NOLINT(bugprone-suspicious-memset-usage)
            this->hashhex[sizeof(this->hashhex)-1] = 0;
        }

        [[nodiscard]] char const* c_str() const noexcept
        {
            return this->hashhex;
        }

        HashHexArray& data() noexcept
        {
            return this->hashhex;
        }

    private:
        HashHexArray hashhex;
    };

    struct ScytaleRandomWrapper
    {
        enum RandomType : bool { LCG, UDEV };

        Random * rnd;

        ScytaleRandomWrapper(RandomType rnd_type)
        {
            switch (rnd_type) {
                case LCG: rnd = new (&u.lcg) LCGRandom(); break;
                case UDEV: rnd = new (&u.udev) URandom(); break;
            }
        }

        ~ScytaleRandomWrapper()
        {
            rnd->~Random();
        }

    private:
        union U {
            LCGRandom lcg; /* for reproductible tests */
            URandom udev;
            char dummy;
            U() : dummy() {}
            ~U() {} /*NOLINT*/
        } u;
    };
} // namespace


extern "C"
{

struct CryptoContextWrapper
{
    CryptoContext cctx;

    CryptoContextWrapper(
        uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
        bool with_encryption, bool with_checksum,
        bool old_encryption_scheme, bool one_shot_encryption_scheme,
        bytes_view master_derivator)
    {
        if (hmac_key) {
            cctx.set_hmac_key(CryptoContext::key_data::from_ptr(hmac_key));
        }
        cctx.set_get_trace_key_cb(trace_fn);
        cctx.set_trace_type(
            with_encryption
            ? TraceType::cryptofile
            : with_checksum
                ? TraceType::localfile_hashed
                : TraceType::localfile);
        cctx.old_encryption_scheme = old_encryption_scheme;
        cctx.one_shot_encryption_scheme = one_shot_encryption_scheme;
        cctx.set_master_derivator(master_derivator);
    }
};

struct ScytaleErrorContext
{
    ScytaleErrorContext() noexcept = default;

    char const * message() noexcept
    {
        if (this->error.errnum) {
            std::snprintf(this->msg, sizeof(msg), "%s, errno = %d: %s",
                this->error.errmsg().c_str(), this->error.errnum, strerror(this->error.errnum));
            this->msg[sizeof(this->msg)-1] = 0;
            return this->msg;
        }

        return this->error.errmsg();
    }

    static char const * handle_get_error_message() noexcept
    {
        return "Handle is nullptr";
    }

    void set_error(Error const & err) noexcept
    {
        this->error = err;
    }

private:
    Error error {NO_ERROR};
    char msg[256];
};

struct ScytaleWriterHandle
{
    ScytaleWriterHandle(
        ScytaleRandomWrapper::RandomType random_type,
        bool with_encryption, bool with_checksum,
        uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
        int old_encryption_scheme , int one_shot_encryption_scheme,
        bytes_view master_derivator)
    : random_wrapper(random_type)
    , cctxw(hmac_key, trace_fn, with_encryption, with_checksum,
            old_encryption_scheme, one_shot_encryption_scheme,
            master_derivator)
    , out_crypto_transport(cctxw.cctx, *random_wrapper.rnd, [](const Error & /*error*/){})
    {}

private:
    ScytaleRandomWrapper random_wrapper;

    CryptoContextWrapper cctxw;

public:
    HashHex qhashhex;
    HashHex fhashhex;

    OutCryptoTransport out_crypto_transport;
    ScytaleErrorContext error_ctx;
};


struct ScytaleReaderHandle
{
    HashHex qhashhex;
    HashHex fhashhex;

    ScytaleReaderHandle(
        InCryptoTransport::EncryptionMode encryption,
        uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
        int old_encryption_scheme, int one_shot_encryption_scheme,
        bytes_view master_derivator)
    : cctxw(hmac_key, trace_fn,
            false /* unused for reading */, false /* unused for reading */,
            old_encryption_scheme, one_shot_encryption_scheme,
            master_derivator)
    , in_crypto_transport(cctxw.cctx, encryption)
    {}

    CryptoContextWrapper cctxw;

    InCryptoTransport in_crypto_transport;
    ScytaleErrorContext error_ctx;
};



const char* scytale_version() {
    return VERSION;
}


/// \return HashHexArray
char const * scytale_writer_get_qhashhex(ScytaleWriterHandle * handle) {
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, "");
    return handle->qhashhex.c_str();
}

/// \return HashHexArray
char const * scytale_writer_get_fhashhex(ScytaleWriterHandle * handle) {
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, "");
    return handle->fhashhex.c_str();
}


ScytaleWriterHandle * scytale_writer_new(
    int with_encryption, int with_checksum,
    uint8_t const * master_derivator, unsigned master_derivator_len,
    uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
    int old_scheme, int one_shot)
{
    SCOPED_TRACE;
    return CREATE_HANDLE(ScytaleWriterHandle(
        ScytaleRandomWrapper::UDEV,
        with_encryption, with_checksum,
        hmac_key, trace_fn,
        old_scheme, one_shot,
        {master_derivator, master_derivator_len}
    ));
}


ScytaleWriterHandle * scytale_writer_new_with_test_random(
    int with_encryption, int with_checksum,
    uint8_t const * master_derivator, unsigned master_derivator_len,
    uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
    int old_scheme, int one_shot)
{
    SCOPED_TRACE;
    return CREATE_HANDLE(ScytaleWriterHandle(
        ScytaleRandomWrapper::LCG,
        with_encryption, with_checksum,
        hmac_key, trace_fn,
        old_scheme, one_shot,
        {master_derivator, master_derivator_len}
    ));
}

int scytale_writer_open(
    ScytaleWriterHandle * handle, char const * record_path, char const * hash_path,
    int permissions, uint8_t const * derivator, unsigned derivator_len)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    handle->error_ctx.set_error(Error(NO_ERROR));
    CHECK_NOTHROW(
        handle->out_crypto_transport.open(
            record_path, hash_path,
            compute_permissions(permissions),
            compute_derivator(record_path, derivator, derivator_len)),
        ERR_TRANSPORT_OPEN_FAILED);
    return 0;
}

int scytale_writer_write(ScytaleWriterHandle * handle, uint8_t const * buffer, unsigned long len) {
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    CHECK_NOTHROW(handle->out_crypto_transport.send(buffer, len), ERR_TRANSPORT_WRITE_FAILED);
    return checked_int{len};
}

int scytale_writer_close(ScytaleWriterHandle * handle) {
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    HashArray qhash;
    HashArray fhash;
    CHECK_NOTHROW(handle->out_crypto_transport.close(qhash, fhash), ERR_TRANSPORT_CLOSED);
    hash_to_hashhex(qhash, handle->qhashhex.data());
    hash_to_hashhex(fhash, handle->fhashhex.data());
    return 0;
}

void scytale_writer_delete(ScytaleWriterHandle * handle)
{
    SCOPED_TRACE;
    delete handle; /*NOLINT*/
}

char const * scytale_writer_get_error_message(ScytaleWriterHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, ScytaleErrorContext::handle_get_error_message());
    return handle->error_ctx.message();
}



ScytaleReaderHandle * scytale_reader_new(
    uint8_t const * master_derivator, unsigned master_derivator_len,
    uint8_t const * hmac_key, get_trace_key_prototype* trace_fn,
    int old_scheme, int one_shot)
{
    SCOPED_TRACE;
    return CREATE_HANDLE(ScytaleReaderHandle(
        InCryptoTransport::EncryptionMode::Auto,
        hmac_key, trace_fn,
        old_scheme, one_shot,
        {master_derivator, master_derivator_len}
    ));
}

int scytale_reader_open(
    ScytaleReaderHandle * handle, char const * path,
    uint8_t const * derivator, unsigned derivator_len
) {
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    handle->error_ctx.set_error(Error(NO_ERROR));
    CHECK_NOTHROW(
        handle->in_crypto_transport.open(
            path,
            compute_derivator(path, derivator, derivator_len)),
        ERR_TRANSPORT_OPEN_FAILED);
    return 0;
}

int scytale_reader_open_with_auto_detect_encryption_scheme(
    ScytaleReaderHandle * handle, char const * path,
    uint8_t const * derivator, unsigned derivator_len)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    handle->error_ctx.set_error(Error(NO_ERROR));
    bytes_view const derivator_array = compute_derivator(path, derivator, derivator_len);
    Error out_error{NO_ERROR};
    CHECK_NOTHROW(
        auto const r = open_if_possible_and_get_encryption_scheme_type(
            handle->in_crypto_transport, path, derivator_array, &out_error);
        switch (r)
        {
            case EncryptionSchemeTypeResult::Error:
                handle->error_ctx.set_error(out_error);
                break;
            case EncryptionSchemeTypeResult::OldScheme:
                // repopen file because some data are lost
                handle->cctxw.cctx.old_encryption_scheme = 1;
                handle->in_crypto_transport.open(path, derivator_array);
                break;
            case EncryptionSchemeTypeResult::NewScheme:
            case EncryptionSchemeTypeResult::NoEncrypted:
                break;
        }
        return int(r),
        ERR_TRANSPORT_OPEN_FAILED);
    return 0;
}

// 0: if end of file, len: if data was read, negative number on error
long long scytale_reader_read(ScytaleReaderHandle * handle, uint8_t * buffer, unsigned long len)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    CHECK_NOTHROW(
        return checked_int(handle->in_crypto_transport.partial_read(buffer, len)),
        ERR_TRANSPORT_READ_FAILED
    );
}

int scytale_reader_send_to(ScytaleReaderHandle * handle, int fd_out)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);

    const int fd_in = handle->in_crypto_transport.get_fd();

    auto set_error = [=](int errnum){
        handle->error_ctx.set_error(Error(ERR_TRANSPORT_WRITE_FAILED, errnum));
        return -1;
    };

    // fast copy for no encrypted file
    if (!handle->in_crypto_transport.is_encrypted()) {
        auto buf = handle->in_crypto_transport.get_and_reset_remaining_buffer();
        size_t pos = 0;
        size_t n = buf.size();
        while (n) {
            ssize_t r = write(fd_out, buf.data() + pos, n);
            // error or EOF for fd_out -> copy fail
            if (r <= 0) {
                return set_error(r ? errno : 0);
            }

            // r > 0
            size_t comsumed = checked_int(r);
            pos += comsumed;
            n -= comsumed;
        }

        for (;;) {
            // use Linux-specific splice(2) for transferring data between arbitrary fds
            auto n = sendfile(fd_out, fd_in, nullptr, 0x7ffff000);
            if (!n) {
                return 0;
            }
            if (n < 0) {
                int errnum = errno;
                // fall back to partial_read()/write()
                if (errnum == EINVAL || errnum == ENOSYS) {
                    break;
                }
                return -1;
            }
        }
    }

    // maybe throw
    auto send_to = [=]{
        struct Free { void operator()(void* p) { std::free(p); } };
        constexpr size_t bufsize = 64 * 1024;
        auto* ptr = static_cast<char*>(std::aligned_alloc(4096, bufsize));
        if (!ptr) {
            return -1;
        }
        std::unique_ptr<char[], Free> guard(ptr);

        writable_buffer_view buf{ptr, bufsize};
        while (size_t n = handle->in_crypto_transport.partial_read(buf)) {
            size_t pos = 0;
            do {
                ssize_t r = write(fd_out, buf.data() + pos, n);
                if (r > 0) {
                    size_t comsumed = checked_int(r);
                    pos += comsumed;
                    n -= comsumed;
                }
                // error or EOF for fd_out -> copy fail
                else if (r <= 0) {
                    return set_error(r ? errno : 0);
                }
            } while (n);
        }

        return 0;
    };

    CHECK_NOTHROW(return send_to(), ERR_TRANSPORT_READ_FAILED);
}

int scytale_reader_close(ScytaleReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    CHECK_NOTHROW(handle->in_crypto_transport.close(), ERR_TRANSPORT_CLOSED);
    return 0;
}

void scytale_reader_delete(ScytaleReaderHandle * handle)
{
    SCOPED_TRACE;
    delete handle; /*NOLINT*/
}

char const * scytale_reader_get_error_message(ScytaleReaderHandle * handle)
{
    CHECK_HANDLE_R(handle, ScytaleErrorContext::handle_get_error_message());
    return handle->error_ctx.message();
}

namespace
{
    int reader_hash(
        ScytaleReaderHandle * handle, const char * filename,
        bool is_quick, HashHex& hash_hex)
    {
        InCryptoTransport::HASH hash;

        bool read_is_ok = is_quick
            ? InCryptoTransport::read_qhash(filename, handle->cctxw.cctx.get_hmac_key(), hash)
            : InCryptoTransport::read_fhash(filename, handle->cctxw.cctx.get_hmac_key(), hash);
        if (!read_is_ok) {
            handle->error_ctx.set_error(Error{ERR_TRANSPORT_READ_FAILED});
            return -1;
        }

        hash_to_hashhex(hash.hash, hash_hex.data());
        return 0;
    }
} // anonymous namespace

int scytale_reader_fhash(ScytaleReaderHandle * handle, const char * filename)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    return reader_hash(handle, filename, false, handle->fhashhex);
}

int scytale_reader_qhash(ScytaleReaderHandle * handle, const char * filename)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    return reader_hash(handle, filename, true, handle->qhashhex);
}


const char * scytale_reader_get_qhashhex(ScytaleReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, nullptr);
    return handle->qhashhex.c_str();
}

const char * scytale_reader_get_fhashhex(ScytaleReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, nullptr);
    return handle->fhashhex.c_str();
}


struct ScytaleMetaReaderHandle
{
    explicit ScytaleMetaReaderHandle(ScytaleReaderHandle & reader)
      : mwrm_reader(reader.in_crypto_transport)
    {}

    ScytaleMwrmHeader & get_header() noexcept
    {
        auto const & header = this->mwrm_reader.get_header();
        this->c_header.version = static_cast<int>(header.version);
        this->c_header.has_checksum = header.has_checksum;
        return this->c_header;
    }

    ScytaleMwrmLine & get_meta_line() noexcept
    {
        if (this->meta_line.with_hash) {
            hash_to_hashhex(this->meta_line.hash1, this->hashhex1);
            hash_to_hashhex(this->meta_line.hash2, this->hashhex2);
        }
        else {
            this->hashhex1[0] = 0;
            this->hashhex2[0] = 0;
        }
        this->c_mwrm_line = {
            this->meta_line.filename,
            static_cast<uint64_t>(this->meta_line.size),
            this->meta_line.mode,
            this->meta_line.uid,
            this->meta_line.gid,
            this->meta_line.dev,
            this->meta_line.ino,
            static_cast<uint64_t>(this->meta_line.mtime),
            static_cast<uint64_t>(this->meta_line.ctime),
            static_cast<uint64_t>(this->meta_line.start_time.count()),
            static_cast<uint64_t>(this->meta_line.stop_time.count()),
            this->meta_line.with_hash,
            this->hashhex1,
            this->hashhex2,
        };
        return this->c_mwrm_line;
    }

private:
    HashHexArray hashhex1;
    HashHexArray hashhex2;

    ScytaleMwrmHeader c_header;
    ScytaleMwrmLine c_mwrm_line;

public:
    MwrmReader mwrm_reader;
    MetaLine meta_line;
    bool eof = false;

    ScytaleErrorContext error_ctx;
};


ScytaleMetaReaderHandle * scytale_meta_reader_new(ScytaleReaderHandle * reader)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(reader, nullptr);
    return CREATE_HANDLE(ScytaleMetaReaderHandle(*reader));
}

char const * scytale_meta_reader_get_error_message(ScytaleMetaReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, ScytaleErrorContext::handle_get_error_message());
    return handle->error_ctx.message();
}

int scytale_meta_reader_read_hash(ScytaleMetaReaderHandle * handle, int version, int has_checksum)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    handle->mwrm_reader.set_header({static_cast<WrmVersion>(version), bool(has_checksum)});
    CHECK_NOTHROW(
        handle->mwrm_reader.read_meta_hash_line(handle->meta_line),
        ERR_TRANSPORT_READ_FAILED);
    return 0;
}

int scytale_meta_reader_read_header(ScytaleMetaReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    CHECK_NOTHROW(
        handle->mwrm_reader.read_meta_headers(),
        ERR_TRANSPORT_READ_FAILED);
    return 0;
}

int scytale_meta_reader_read_line(ScytaleMetaReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    CHECK_NOTHROW(
        handle->eof
            = Transport::Read::Eof == handle->mwrm_reader.read_meta_line(handle->meta_line),
        ERR_TRANSPORT_READ_FAILED);
    return handle->eof ? ERR_TRANSPORT_NO_MORE_DATA : NO_ERROR;
}

int scytale_meta_reader_read_line_eof(ScytaleMetaReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    return handle->eof;
}

void scytale_meta_reader_delete(ScytaleMetaReaderHandle * handle)
{
    SCOPED_TRACE;
    delete handle; /*NOLINT*/
}

ScytaleMwrmHeader * scytale_meta_reader_get_header(ScytaleMetaReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, nullptr);
    return &handle->get_header();
}

ScytaleMwrmLine * scytale_meta_reader_get_line(ScytaleMetaReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, nullptr);
    return &handle->get_meta_line();
}


struct ScytaleTflWriterHandler
{
    FdxCapture::TflFile tfl_file;
    std::string original_filename;
    ScytaleFdxWriterHandle& fdx;
};

struct ScytaleFdxWriterHandle
{
    ScytaleFdxWriterHandle(
        int with_encryption, int with_checksum, bytes_view master_derivator,
        uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
        ScytaleRandomWrapper::RandomType random_type,
        char const * record_path, char const * hash_path, char const * fdx_file_base,
        char const * sid, FilePermissions permissions)
    : random_wrapper(random_type)
    , cctxw(hmac_key, trace_fn, with_encryption, with_checksum, false, false, master_derivator)
    , fdx_capture(record_path, hash_path, fdx_file_base, sid, permissions,
        this->cctxw.cctx, *this->random_wrapper.rnd, [](const Error &/*error*/){})
    {
        this->qhashhex[0] = 0;
        this->fhashhex[0] = 0;
    }

    char const * get_fdx_path() const noexcept
    {
        return this->fdx_capture.get_fdx_path();
    }

    ScytaleTflWriterHandler * open_tfl(char const* filename, int direction)
    {
        auto* tfl = new(std::nothrow) ScytaleTflWriterHandler{ /*NOLINT*/
            this->fdx_capture.new_tfl(checked_int{direction}),
            filename,
            *this,
        };

        if (not tfl)
        {
            this->error_ctx.set_error(Error(ERR_MEMORY_ALLOCATION_FAILED));
        }

        return tfl;
    }

    int close_tfl(ScytaleTflWriterHandler& tfl, int transfered_status, bytes_view sig)
    {
        this->fdx_capture.close_tfl(tfl.tfl_file, tfl.original_filename,
            checked_int{transfered_status}, Mwrm3::Sha256Signature{sig});
        return 0;
    }

    int cancel_tfl(ScytaleTflWriterHandler& tfl)
    {
        this->fdx_capture.cancel_tfl(tfl.tfl_file);
        return 0;
    }

    int close()
    {
        if (fdx_capture.is_open())
        {
            HashArray qhash;
            HashArray fhash;
            this->fdx_capture.close(qhash, fhash);
            hash_to_hashhex(qhash, this->qhashhex);
            hash_to_hashhex(fhash, this->fhashhex);
            return 0;
        }

        return 1;
    }

private:
    ScytaleRandomWrapper random_wrapper;
    CryptoContextWrapper cctxw;

    FdxCapture fdx_capture;

public:
    ScytaleErrorContext error_ctx;
    HashHexArray qhashhex;
    HashHexArray fhashhex;
};


ScytaleFdxWriterHandle * scytale_fdx_writer_new(
    int with_encryption, int with_checksum,
    uint8_t const * master_derivator, unsigned master_derivator_len,
    uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
    char const * record_path, char const * hash_path, char const * fdx_file_base,
    char const * sid, int permissions)
{
    SCOPED_TRACE;
    return CREATE_HANDLE(ScytaleFdxWriterHandle(
        with_encryption, with_checksum,
        {master_derivator, master_derivator_len},
        hmac_key, trace_fn, ScytaleRandomWrapper::UDEV,
        record_path, hash_path, fdx_file_base, sid,
        compute_permissions(permissions)));
}

ScytaleFdxWriterHandle * scytale_fdx_writer_new_with_test_random(
    int with_encryption, int with_checksum,
    uint8_t const * master_derivator, unsigned master_derivator_len,
    uint8_t const * hmac_key, get_trace_key_prototype * trace_fn,
    char const * record_path, char const * hash_path, char const * fdx_file_base,
    char const * sid, int permissions)
{
    SCOPED_TRACE;
    return CREATE_HANDLE(ScytaleFdxWriterHandle(
        with_encryption, with_checksum,
        {master_derivator, master_derivator_len},
        hmac_key, trace_fn, ScytaleRandomWrapper::LCG,
        record_path, hash_path, fdx_file_base, sid,
        compute_permissions(permissions)));
}

char const * scytale_fdx_get_path(ScytaleFdxWriterHandle * handle)
{
    SCOPED_TRACE;
    return handle ? handle->get_fdx_path() : nullptr;
}

ScytaleTflWriterHandler * scytale_fdx_writer_open_tfl(
    ScytaleFdxWriterHandle * handle, char const * filename, int direction)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, nullptr);
    CHECK_NOTHROW_R(return handle->open_tfl(filename, direction),
        nullptr, handle->error_ctx, ERR_TRANSPORT_OPEN_FAILED);
}

int scytale_tfl_writer_write(
    ScytaleTflWriterHandler * handle, uint8_t const * buffer, unsigned long len)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    CHECK_NOTHROW_R(handle->tfl_file.trans.send(buffer, len),
        -1, handle->fdx.error_ctx, ERR_TRANSPORT_WRITE_FAILED);
    return checked_int{len};
}

int scytale_tfl_writer_close(
    ScytaleTflWriterHandler * handle, int transfered_status,
    uint8_t const* sig, unsigned long len)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    std::unique_ptr<ScytaleTflWriterHandler> auto_delete{handle};
    CHECK_NOTHROW_R(return handle->fdx.close_tfl(
        *handle, transfered_status, bytes_view{sig, len}
    ), -1, handle->fdx.error_ctx, ERR_TRANSPORT_WRITE_FAILED);
}

int scytale_tfl_writer_cancel(ScytaleTflWriterHandler * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    std::unique_ptr<ScytaleTflWriterHandler> auto_delete{handle};
    return handle->fdx.cancel_tfl(*handle);
}

/// \return HashHexArray
char const * scytale_fdx_writer_get_qhashhex(ScytaleFdxWriterHandle * handle) {
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, "");
    return handle->qhashhex;
}

/// \return HashHexArray
char const * scytale_fdx_writer_get_fhashhex(ScytaleFdxWriterHandle * handle) {
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, "");
    return handle->fhashhex;
}

int scytale_fdx_writer_close(ScytaleFdxWriterHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE(handle);
    CHECK_NOTHROW_R(return handle->close(), -1, handle->error_ctx, ERR_TRANSPORT_CLOSED);
}

int scytale_fdx_writer_delete(ScytaleFdxWriterHandle * handle)
{
    SCOPED_TRACE;
    delete handle; /*NOLINT*/
    return 0;
}

char const * scytale_fdx_writer_get_error_message(ScytaleFdxWriterHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, ScytaleErrorContext::handle_get_error_message());
    return handle->error_ctx.message();
}

} // extern "C"

namespace
{
    struct scytale_bytes_view
    {
        uint8_t const* ptr;
        uint32_t size;
    };

    struct scytale_str_view
    {
        char const* ptr;
        uint32_t size;
    };

    template<class T>
    constexpr char char_type_fmt()
    {
        if constexpr (std::is_same_v<T, scytale_bytes_view>)
        {
            return 'B';
        }
        else if constexpr (std::is_same_v<T, scytale_str_view>)
        {
            return 's';
        }
        else if constexpr (std::is_unsigned_v<T>)
        {
            return 'u';
        }
        else if constexpr (std::is_signed_v<T>)
        {
            return 'i';
        }
        else
        {
            assert(false);
        }
    }

    template<class... xs>
    using string_type_fmt = jln::string_c<char_type_fmt<xs>()...>;

    template<std::size_t i, class T>
    struct tuple_elem
    {
        T value;
    };

    template<class... xs>
    struct tuple_t : xs... {};

    template<class Ints, class... xs>
    struct tuple_impl;

    template<std::size_t... ints, class... Ts>
    struct tuple_impl<std::integer_sequence<size_t, ints...>, Ts...>
    {
        using type = tuple_t<tuple_elem<ints, Ts>...>;
    };

    // compatibility C layout
    template<class... xs>
    using tuple = typename tuple_impl<std::index_sequence_for<xs...>, xs...>::type;

    template<class Type, class... Ts>
    struct storage_params
    {
        using fmt = string_type_fmt<Ts...>;
        using mwrm3_type = Type;
        using storage_type = tuple<Ts...>;
    };

    template<class... Ts>
    struct storage_list
    {
        storage_list() = default;

        template<class... Us>
        storage_list(storage_list<Us...> /*dummy*/) noexcept
        {}

        static const std::size_t size = sizeof...(Ts);
    };

    // storage list for error (test ? storage_list<xs...> : storage_list<>) -> storage_list<xs...>
    template<>
    struct storage_list<>
    {};

    template<class... StorageParams>
    struct recursive_storage_union
    {
        struct type
        {
            tuple_t<> storage;
        };
    };

    template<class StorageParams0, class ... others>
    struct recursive_storage_union<StorageParams0, others...>
    {
        union type
        {
            using storage_params = StorageParams0;

            typename storage_params::storage_type storage;
            typename recursive_storage_union<others...>::type next;

            type() = default;
        };
    };

    // fastpath
    template<
        class StorageParams0,
        class StorageParams1,
        class StorageParams2,
        class StorageParams3,
        class... others>
    struct recursive_storage_union<
        StorageParams0,
        StorageParams1,
        StorageParams2,
        StorageParams3,
        others...>
    {
        union type
        {
            using storage_params = StorageParams0;

            typename storage_params::storage_type storage;
            union type1
            {
                using storage_params = StorageParams1;

                typename storage_params::storage_type storage;
                union type2
                {
                    using storage_params = StorageParams2;

                    typename storage_params::storage_type storage;
                    union type3
                    {
                        using storage_params = StorageParams3;

                        typename storage_params::storage_type storage;
                        typename recursive_storage_union<others...>::type next;

                        type3() = default;
                    } next;

                    type2() = default;
                } next;

                type1() = default;
            } next;

            type() = default;
        };
    };

    template<unsigned i, class Union>
    auto& get_union_element(Union& u)
    {
        if constexpr (i >= 8)
        {
            return get_union_element<i-8>(u.next.next.next.next.next.next.next.next);
        }
        else if constexpr (i >= 4)
        {
            return get_union_element<i-4>(u.next.next.next.next);
        }
        else if constexpr (i == 3)
        {
            return u.next.next.next;
        }
        else if constexpr (i == 2)
        {
            return u.next.next;
        }
        else if constexpr (i == 1)
        {
            return u.next;
        }
        else
        {
            return u;
        }
    }

    template<class L>
    struct storage_variant;

    template<class... StorageParams>
    struct storage_variant<storage_list<StorageParams...>>
    {
        using union_type = typename recursive_storage_union<
            StorageParams...,
            storage_params<void>
        >::type;

        constexpr static unsigned get_index(Mwrm3::Type type)
        {
            unsigned i = 0;
            (void)(... && (StorageParams::mwrm3_type::value != type && ++i));
            return i;
        }

        template<class Mwrm3Type>
        auto& operator[](Mwrm3Type type)
        {
            return get_union_element<get_index(type)>(this->u);
        }

        union_type u;
    };
} // anonymous namespace

namespace std
{
    template<class... Ts, class... Us>
    struct common_type<storage_list<Ts...>, storage_list<Us...>>
    {
        using type = storage_list<Ts..., Us...>;
    };
}

namespace
{
    template<class T>
    auto scytale_raw_integral(T x)
    {
        if constexpr (std::is_unsigned_v<T>)
        {
            return uint64_t(x);
        }
        else
        {
            return int64_t(x);
        }
    }

    template<class T, class = void>
    struct scytale_raw_value_impl
    {
        static_assert(!std::is_same<T, T>::value,
            "missing specialization or not a regular type"
            " (should be a struct with bytes or str member, enum, integral or std::chrono::duration)");
    };

    template<class T>
    struct scytale_raw_value_impl<T, decltype(void(std::declval<T>().bytes))>
    {
        static auto raw(T const& x)
        {
            return scytale_bytes_view{x.bytes.data(), checked_int(x.bytes.size())};
        }
    };

    template<class T>
    struct scytale_raw_value_impl<T, decltype(void(std::declval<T>().str))>
    {
        static auto raw(T const& x)
        {
            return scytale_str_view{x.str.data(), checked_int(x.str.size())};
        }
    };

    template<class Rep, class Period>
    struct scytale_raw_value_impl<std::chrono::duration<Rep, Period>, void>
    {
        static auto raw(std::chrono::duration<Rep, Period> const& duration)
        {
            return scytale_raw_integral(duration.count());
        }
    };

    template<class T>
    auto scytale_raw_value(T const& x)
    {
        if constexpr (std::is_enum_v<T>)
        {
            return scytale_raw_integral(std::underlying_type_t<T>(x));
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return scytale_raw_integral(x);
        }
        else
        {
            return scytale_raw_value_impl<T>::raw(x);
        }
    }

    REDEMPTION_DIAGNOSTIC_PUSH()
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wunneeded-internal-declaration")
    // returns a storage_list of storage_params
    inline auto make_storage_list()
    {
        auto bind_params = [](auto type, bytes_view /*remaining*/, auto... xs){
            return storage_list<storage_params<
                decltype(type), decltype(scytale_raw_value(xs))...
            >>();
        };

        return Mwrm3::unserialize_packet(Mwrm3::Type::None, {}, bind_params, [](Mwrm3::Type /*type*/){
            return storage_list<>();
        });
    }
    REDEMPTION_DIAGNOSTIC_POP()
} // anonymous namespace

extern "C"
{

struct ScytaleMwrm3ReaderHandle
{
    ScytaleMwrm3ReaderHandle(ScytaleReaderHandle& reader)
    : reader(reader)
    {}

    char const* get_error() const noexcept
    {
        return this->message_error.c_str();
    }

    ScytaleMwrm3ReaderData const* next()
    {
        auto store_values = [this](auto type, bytes_view remaining_data, auto... xs){
            auto& union_element = this->storage_variant[type];
            using UnionElement = std::remove_reference_t<decltype(union_element)>;
            using StorageParams = typename UnionElement::storage_params;

            this->raw_data.type = safe_int{type.value};
            this->raw_data.fmt = StorageParams::fmt::c_str();
            this->raw_data.data = new(&union_element.storage) /*NOLINT*/
                typename StorageParams::storage_type{{scytale_raw_value(xs)}...};

            this->remaining_data = remaining_data;
        };

        auto read_next = [&]() -> ScytaleMwrm3ReaderData * {
            for (;;)
            {
                switch (Mwrm3::parse_packet(this->remaining_data, store_values))
                {
                    case Mwrm3::ParserResult::Ok:
                        return &this->raw_data;

                    case Mwrm3::ParserResult::UnknownType: {
                        auto int_type = InStream(this->remaining_data).in_uint16_le();
                        auto chars = int_to_fixed_hexadecimal_upper_chars(int_type);
                        str_assign(this->message_error, "Unknown type: 0x"_av, chars);
                        return nullptr;
                    }

                    case Mwrm3::ParserResult::NeedMoreData: {
                        auto reader = [&](writable_bytes_view data){ return this->reader.in_crypto_transport.partial_read(data); };
                        auto result = move_remaining_data(remaining_data, buffer, reader);
                        switch (result.status)
                        {
                            case RemainingDataResult::Status::Ok:
                                remaining_data = result.data;
                                break;

                            case RemainingDataResult::Status::Eof:
                                return nullptr;

                            case RemainingDataResult::Status::DataToLarge:
                                this->message_error = "Data too large";
                                return nullptr;
                        }
                    }
                }
            }
        };

        auto set_error = [&]{
            str_assign(this->message_error,
                "Read error: ", this->reader.error_ctx.message());
            return nullptr;
        };

        CHECK_NOTHROW_R(return read_next(), set_error(), this->reader.error_ctx, ERR_TRANSPORT_READ_FAILED);
    }

private:
    ScytaleReaderHandle& reader;
    bytes_view remaining_data {"", 0};

    using storage_list_t = decltype(make_storage_list());

    ScytaleMwrm3ReaderData raw_data;

    ::storage_variant<storage_list_t> storage_variant;

    std::string message_error;

    std::array<uint8_t, 1024*16> buffer;
};

ScytaleMwrm3ReaderHandle * scytale_mwrm3_reader_new(ScytaleReaderHandle * reader)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(reader, nullptr);
    return CREATE_HANDLE(ScytaleMwrm3ReaderHandle(*reader));
}

ScytaleMwrm3ReaderData const* scytale_mwrm3_reader_read_next(ScytaleMwrm3ReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, nullptr);
    return handle->next();
}

char const * scytale_mwrm3_reader_get_error_message(ScytaleMwrm3ReaderHandle * handle)
{
    SCOPED_TRACE;
    CHECK_HANDLE_R(handle, ScytaleErrorContext::handle_get_error_message());
    return handle->get_error();
}

int scytale_mwrm3_reader_delete(ScytaleMwrm3ReaderHandle * handle)
{
    SCOPED_TRACE;
    delete handle; /*NOLINT*/
    return 0;
}

}
