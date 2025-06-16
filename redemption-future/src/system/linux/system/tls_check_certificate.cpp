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
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan

   Transport layer abstraction, socket implementation with TLS support
*/

#include "core/error.hpp"
#include "system/tls_check_certificate.hpp"
#include "utils/file.hpp"
#include "utils/fileutils.hpp"
#include "utils/log.hpp"
#include "utils/strutils.hpp"
#include "utils/sugar/int_to_chars.hpp"
#include "utils/sugar/scope_exit.hpp"

#include <string>
#include <new>
#include <memory_resource>
#include <cstring>
#include <cstdlib>

#include <fcntl.h>

#include "cxx/diagnostic.hpp"
#include "cxx/cxx.hpp"

#include <openssl/ssl.h>
#include <openssl/x509.h>


REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_GCC_IGNORE("-Wold-style-cast")
REDEMPTION_DIAGNOSTIC_GCC_ONLY_IGNORE("-Wzero-as-null-pointer-constant")
#if REDEMPTION_WORKAROUND(REDEMPTION_COMP_CLANG, >= 500)
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wzero-as-null-pointer-constant")
#endif

namespace
{

struct TLSCheckCertificateResource
{
    zstring_view printable_issuer_name(X509 const* x509)
    {
        return printable_name(X509_get_issuer_name(x509));
    }

    zstring_view printable_subject_name(X509 const* x509)
    {
        return printable_name(X509_get_subject_name(x509));
    }

    zstring_view cert_fingerprint(X509 const* xcert)
    {
        uint32_t fp_len = 0;
        uint8_t fp[EVP_MAX_MD_SIZE];

        X509_digest(xcert, EVP_sha1(), fp, &fp_len);

        if (!fp_len) {
            return zstring_view();
        }

        auto buf = alloc_charp(3u * fp_len);
        char* p = buf.data();

        for (uint8_t c : bytes_view{fp, fp_len}) {
            p = int_to_fixed_hexadecimal_upper_chars(p, c);
            *p++ = ':';
        }
        // replace last ':' with zero-terminal
        buf.back() = '\0';

        return zstring_view::from_null_terminated(buf.drop_back(1));
    }

    chars_view get_certificate(X509 const* xcert)
    {
        auto* outBIO = bio();
        if (!PEM_write_bio_X509(outBIO, xcert)) {
            return {};
        }

        return read_bio(outBIO, 0);
    }

    writable_chars_view alloc_charp(std::size_t n)
    {
        char* p = std::launder(static_cast<char*>(_mbr.allocate(n, 1)));
        return writable_chars_view{p, n};
    }

    ~TLSCheckCertificateResource()
    {
        if (_bio) {
            BIO_free(_bio);
        }
    }

private:
    std::pmr::monotonic_buffer_resource _mbr;
    BIO* _bio = nullptr;
    uint64_t bio_len = 0;

    BIO* bio()
    {
        if (REDEMPTION_UNLIKELY(!_bio)) {
            _bio = BIO_new(BIO_s_mem());
            if (!_bio) {
                throw Error(ERR_MEMORY_ALLOCATION_FAILED);
            }
        }
        return _bio;
    }

    writable_chars_view read_bio(BIO* outBIO, std::size_t extra_allocated)
    {
        auto len = BIO_number_written(outBIO) - bio_len;
        bio_len += len;
        auto buf = alloc_charp(len + extra_allocated);
        BIO_read(outBIO, buf.data(), checked_int(len));
        return buf;
    }

    zstring_view printable_name(X509_NAME const* name)
    {
        auto* outBIO = bio();
        if (X509_NAME_print_ex(outBIO, name, 0, XN_FLAG_ONELINE) > 0) {
            auto buf = read_bio(outBIO, 1);
            buf.back() = '\0';
            return zstring_view::from_null_terminated(buf.drop_back(1));
        }

        return zstring_view();
    }
};

} // anonymous namespace

[[nodiscard]] bool tls_check_certificate(
    X509& x509,
    bool server_cert_store,
    ServerCertCheck server_cert_check,
    BasicFunction<void(CertificateStatus status, std::string_view error_msg)> certificate_checker,
    const char* certif_path,
    const char* protocol,
    const char* ip_address,
    int port)
{
    TLSCheckCertificateResource resource;

    const bool ensure_server_certificate_match
      = server_cert_check == ServerCertCheck::fails_if_no_match_or_missing
     || server_cert_check == ServerCertCheck::fails_if_no_match_and_succeed_if_no_know;

    const bool ensure_server_certificate_exists
      = server_cert_check == ServerCertCheck::fails_if_no_match_or_missing
     || server_cert_check == ServerCertCheck::succeed_if_exists_and_fails_if_missing;

    bool bad_certificate_path = false;
    error_type checking_exception = NO_ERROR;

    // ensures the certificate directory exists
    LOG(LOG_INFO, "certificate directory is: '%s'", certif_path);
    if (recursive_create_directory(certif_path, S_IRWXU|S_IRWXG) != 0) {
        int const errnum = errno;
        LOG(LOG_WARNING, "Failed to create certificate directory: %s ", certif_path);
        bad_certificate_path = true;

        certificate_checker(CertificateStatus::CertError, strerror(errnum));
        checking_exception = ERR_TRANSPORT_TLS_CERTIFICATE_INACCESSIBLE;
    }

    char filename[1024];

    // generates the name of certificate file associated with RDP target
    snprintf(filename, sizeof(filename) - 1, "%s/%s,%s,%d,X509.pem",
        certif_path, protocol, ip_address, port);
    filename[sizeof(filename) - 1] = '\0';
    LOG(LOG_INFO, "certificate file is: '%s'", filename);

    bool certificate_exists  = false;
    bool certificate_matches = false;

    if (!bad_certificate_path) {
        File fp(filename, "r");
        if (!fp) {
            switch (int const errnum = errno) {
            default: {
                // failed to open stored certificate file
                LOG(LOG_WARNING, "Failed to open stored certificate: \"%s\"", filename);
                certificate_checker(CertificateStatus::CertError, strerror(errnum));
                checking_exception = ERR_TRANSPORT_TLS_CERTIFICATE_INACCESSIBLE;
            }
            break;
            case ENOENT:
            {
                LOG(LOG_WARNING, "There's no stored certificate: \"%s\"", filename);
                if (ensure_server_certificate_exists) {
                    certificate_checker(CertificateStatus::CertFailure, {});
                    checking_exception = ERR_TRANSPORT_TLS_CERTIFICATE_MISSED;
                }
            }
            break;
            }
        } // fp
        else {
            certificate_exists  = true;
            X509 *px509Existing = PEM_read_X509(fp.get(), nullptr, nullptr, nullptr);
            int const errnum = px509Existing ? 0 : errno;
            fp.close();
            if (!px509Existing) {
                // failed to read stored certificate file
                LOG(LOG_WARNING, "Failed to read stored certificate: \"%s\"", filename);
                certificate_checker(CertificateStatus::CertError, strerror(errnum));
                checking_exception = ERR_TRANSPORT_TLS_CERTIFICATE_CORRUPTED;
            }
            else {
                SCOPE_EXIT(X509_free(px509Existing));

                certificate_matches = [&]{
                    auto certificate = resource.get_certificate(&x509);
                    File f1(filename, "r");
                    // len + 1 for detected EOF
                    auto buf = f1.read(resource.alloc_charp(certificate.size() + 1));
                    return buf.size() == certificate.size()
                        && f1.is_eof()
                        && 0 == memcmp(certificate.data(), buf.data(), certificate.size());
                }();

                const auto issuer_existing = resource.printable_issuer_name(px509Existing);
                const auto subject_existing = resource.printable_subject_name(px509Existing);
                const auto fingerprint_existing = resource.cert_fingerprint(px509Existing);

                LOG(LOG_INFO, "TLS::X509 existing::issuer=%s", issuer_existing);
                LOG(LOG_INFO, "TLS::X509 existing::subject=%s", subject_existing);
                LOG(LOG_INFO, "TLS::X509 existing::fingerprint=%s", fingerprint_existing);

                bool certificate_check_success = true;
                if (!certificate_matches) {
                    const auto issuer = resource.printable_issuer_name(&x509);
                    const auto subject = resource.printable_subject_name(&x509);
                    const auto fingerprint = resource.cert_fingerprint(&x509);

                    if (// Read certificate fields to ensure change is not irrelevant
                        // Relevant changes are either:
                        // - issuer changed
                        // - subject changed (or appears, or disappears)
                        // - fingerprint changed
                        // other changes are ignored (expiration date for instance,
                        //  and revocation list is not checked)
                           ((issuer_existing.to_sv() != issuer.to_sv())
                        // Only one of subject_existing and subject is null
                        || (subject_existing.empty() ^ subject.empty())
                        // All of subject_existing and subject are not null
                        || (!subject.empty() && subject_existing.to_sv() != subject.to_sv())
                        || (fingerprint_existing.to_sv() != fingerprint.to_sv()))
                    ) {
                        LOG(LOG_WARNING, "The certificate for host %s:%d has changed Previous=\"%s\" \"%s\" \"%s\", New=\"%s\" \"%s\" \"%s\"",
                            ip_address, port,
                            issuer_existing, subject_existing,
                            fingerprint_existing, issuer,
                            subject, fingerprint);
                        if (ensure_server_certificate_match) {
                            certificate_checker(CertificateStatus::CertFailure, {});
                            checking_exception = ERR_TRANSPORT_TLS_CERTIFICATE_CHANGED;
                        }
                        certificate_check_success = false;
                    }
                }

                if (certificate_check_success) {
                    certificate_checker(CertificateStatus::CertSuccess, {});
                }
            }
        }
    }

    if ((certificate_exists || !ensure_server_certificate_exists)
    && (!certificate_exists || certificate_matches || !ensure_server_certificate_match))
    {
        if ((!certificate_exists || !certificate_matches) && server_cert_store) {
            ::unlink(filename);

            LOG(LOG_INFO, "Dumping X509 peer certificate: \"%s\"", filename);
            if (File fp{filename, "w+"}) {
                PEM_write_X509(fp.get(), &x509);
                fp.close();
                LOG(LOG_INFO, "Dumped X509 peer certificate");
                certificate_checker(CertificateStatus::CertCreate, {});
            }
            else {
                LOG(LOG_WARNING, "Failed to dump X509 peer certificate");
            }
        }

        certificate_checker(CertificateStatus::AccessAllowed, {});

        // SSL_get_verify_result();

        // SSL_get_verify_result - get result of peer certificate verification
        // -------------------------------------------------------------------
        // SSL_get_verify_result() returns the result of the verification of the X509 certificate
        // presented by the peer, if any.

        // SSL_get_verify_result() can only return one error code while the verification of
        // a certificate can fail because of many reasons at the same time. Only the last
        // verification error that occurred during the processing is available from
        // SSL_get_verify_result().

        // The verification result is part of the established session and is restored when
        // a session is reused.

        // bug: If no peer certificate was presented, the returned result code is X509_V_OK.
        // This is because no verification error occurred, it does however not indicate
        // success. SSL_get_verify_result() is only useful in connection with SSL_get_peer_certificate(3).

        // RETURN VALUES The following return values can currently occur:

        // X509_V_OK: The verification succeeded or no peer certificate was presented.
        // Any other value: Documented in verify(1).


        //  A X.509 certificate is a structured grouping of information about an individual,
        // a device, or anything one can imagine. A X.509 CRL (certificate revocation list)
        // is a tool to help determine if a certificate is still valid. The exact definition
        // of those can be found in the X.509 document from ITU-T, or in RFC3280 from PKIX.
        // In OpenSSL, the type X509 is used to express such a certificate, and the type
        // X509_CRL is used to express a CRL.

        // A related structure is a certificate request, defined in PKCS#10 from RSA Security,
        // Inc, also reflected in RFC2896. In OpenSSL, the type X509_REQ is used to express
        // such a certificate request.

        // To handle some complex parts of a certificate, there are the types X509_NAME
        // (to express a certificate name), X509_ATTRIBUTE (to express a certificate attributes),
        // X509_EXTENSION (to express a certificate extension) and a few more.

        // Finally, there's the supertype X509_INFO, which can contain a CRL, a certificate
        // and a corresponding private key.

        X509_STORE* cert_ctx = X509_STORE_new();

        // OpenSSL_add_all_algorithms(3SSL)
        // --------------------------------

        // OpenSSL keeps an internal table of digest algorithms and ciphers. It uses this table
        // to lookup ciphers via functions such as EVP_get_cipher_byname().

        // OpenSSL_add_all_digests() adds all digest algorithms to the table.

        // OpenSSL_add_all_algorithms() adds all algorithms to the table (digests and ciphers).

        // OpenSSL_add_all_ciphers() adds all encryption algorithms to the table including password
        // based encryption algorithms.

        OpenSSL_add_all_algorithms();

        X509_LOOKUP* lookup = X509_STORE_add_lookup(cert_ctx, X509_LOOKUP_file());
        lookup = X509_STORE_add_lookup(cert_ctx, X509_LOOKUP_hash_dir());

        X509_LOOKUP_add_dir(lookup, nullptr, X509_FILETYPE_DEFAULT);
        //X509_LOOKUP_add_dir(lookup, certificate_store_path, X509_FILETYPE_ASN1);

        X509_STORE_CTX* csc = X509_STORE_CTX_new();
        X509_STORE_set_flags(cert_ctx, 0);
        X509_STORE_CTX_init(csc, cert_ctx, &x509, nullptr);
        X509_verify_cert(csc);
        X509_STORE_CTX_free(csc);

        X509_STORE_free(cert_ctx);

        // int index = X509_NAME_get_index_by_NID(subject_name, NID_commonName, -1);
        // X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject_name, index);
        // ASN1_STRING * entry_data = X509_NAME_ENTRY_get_data(entry);
        // void * subject_alt_names = X509_get_ext_d2i(xcert, NID_subject_alt_name, 0, 0);

        LOG(LOG_INFO, "TLS::X509::issuer=%s", resource.printable_issuer_name(&x509));
        LOG(LOG_INFO, "TLS::X509::subject=%s", resource.printable_subject_name(&x509));
        LOG(LOG_INFO, "TLS::X509::fingerprint=%s", resource.cert_fingerprint(&x509));
    }
    else {
        throw Error(checking_exception);
    }

    return true;
}

REDEMPTION_DIAGNOSTIC_POP()
