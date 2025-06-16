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
  Copyright (C) Wallix 2013
  Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan
*/


#pragma once

#ifndef __EMSCRIPTEN__

#include <string_view>

#include "core/RDP/nla/credssp.hpp"
#include "core/RDP/nla/kerberos.hpp"
#include "utils/fileutils.hpp"
#include "utils/hexdump.hpp"
#include "utils/random.hpp"
#include "system/ssl_sha256.hpp"

#include "transport/transport.hpp"


///
class SEC_WINNT_AUTH_IDENTITY
{
private:
    ///
    std::string username_utf8;

    ///
    std::string username_utf16;

    ///
    std::vector<uint8_t> domain_utf8;

    ///
    std::vector<uint8_t> domain_utf16;

    ///
    std::vector<uint8_t> password_utf8;

    ///
    std::vector<uint8_t> password_utf16;

    ///
    struct
    {
        ///
        std::string keytab_file_path;
    }
    kerberos;

public:
    SEC_WINNT_AUTH_IDENTITY() = default;

/*
    bool is_empty_user_domain() const
    {
        return (this->username_utf16.empty() && this->domain_utf16.empty());
    }
*/

    bool is_valid() const
    {
        return (
            !this->username_utf16.empty()/* &&
            !this->domain_utf16.empty()*/
        );
    }

    void set_kerberos_keytab_path(std::string_view value)
    {
        this->kerberos.keytab_file_path = value;
    }

    const char * get_kerberos_principal_name() const
    {
        return this->username_utf8.c_str();
    }

    const char * get_kerberos_password() const
    {
        if (this->password_utf8.data() == nullptr)
        {
            return nullptr;
        }

        if (this->password_utf8.empty())
        {
            return "";
        }

        return char_ptr_cast(this->password_utf8.data());
    }

    const char * get_kerberos_keytab_path() const
    {
        return this->kerberos.keytab_file_path.c_str();
    }

    bool has_kerberos_keytab() const
    {
        return file_exist(this->kerberos.keytab_file_path);
    }

/*
    bytes_view get_username_utf8() const
    {
        return this->username_utf8;
    }
*/

    bytes_view get_username_utf16() const
    {
        return this->username_utf16;
    }

/*
    bytes_view get_password_utf8() const
    {
        return this->password_utf8;
    }
*/

    bytes_view get_password_utf16() const
    {
        return this->password_utf16;
    }

/*
    bytes_view get_domain_utf8() const
    {
        return this->domain_utf8;
    }
*/

    bytes_view get_domain_utf16() const
    {
        return this->domain_utf16;
    }

    void set_username_from_utf8(bytes_view username)
    {
        this->username_utf8.assign(username.begin(), username.end());
        ::UTF8toResizableUTF16(username, this->username_utf16);
    }

    void set_domain_from_utf8(bytes_view domain)
    {
        this->domain_utf8.assign(domain.begin(), domain.end());
        ::UTF8toResizableUTF16(domain, this->domain_utf16);
    }

    void set_password_from_utf8(const uint8_t *password)
    {
        if (password)
        {
            const std::size_t password_length_utf16 = UTF8Len(password) << 1;

            this->password_utf8.assign(password, password + ::strlen(char_ptr_cast(password)) + 1);
            this->password_utf16.resize(password_length_utf16);
            UTF8toUTF16({password, std::strlen(char_ptr_cast(password))},
                this->password_utf16.data(), this->password_utf16.size());
        }
        else
        {
            this->password_utf8.clear();
            this->password_utf16.clear();
        }
    }

    void clear()
    {
        this->username_utf8.clear();
        this->username_utf16.clear();
        this->domain_utf8.clear();
        this->domain_utf16.clear();
        this->password_utf8.clear();
        this->password_utf16.clear();
    }
};

class rdpCredsspClientKerberos
{
private:
    int send_seq_num = 0;
    int recv_seq_num = 0;

    TSCredentials ts_credentials;
    TSRequest ts_request;
    uint32_t error_code = 0;

    ClientNonce SavedClientNonce;

    std::vector<uint8_t> PublicKey;
    std::vector<uint8_t> ClientServerHash;
    std::vector<uint8_t> ServerClientHash;
    std::string ServicePrincipalName;

    struct Krb5Creds_deleter
    {
        void operator()(Krb5Creds* credentials) const
        {
            credentials->destroy_credentials(nullptr);
            delete credentials; /*NOLINT*/
        }
    };

    using Krb5CredsPtr = std::unique_ptr<Krb5Creds, Krb5Creds_deleter>;

    ///
    SEC_WINNT_AUTH_IDENTITY identity;

    ///
    SEC_WINNT_AUTH_IDENTITY service_identity;

    Krb5CredsPtr sspi_credentials = nullptr;
    std::unique_ptr<KERBEROSContext> sspi_krb_ctx = nullptr;

    // GSS_Acquire_cred
    // ACQUIRE_CREDENTIALS_HANDLE_FN AcquireCredentialsHandle;
    SEC_STATUS sspi_AcquireCredentialsHandle(
        const std::string & pszPrincipal, std::string & pvLogonID, SEC_WINNT_AUTH_IDENTITY const* identity,
        SEC_WINNT_AUTH_IDENTITY const* service_identity
    ) {
        pvLogonID = pszPrincipal;
        this->sspi_credentials = Krb5CredsPtr(new Krb5Creds);

        const int pid(getpid());
        char cache_name[256];
        char fast_cache_name[256];
        const char *fast_cache_name_ptr(nullptr);
        int ret(-1);

        // retrieve and cache credentials of a possible service identity
        if (service_identity && service_identity->is_valid())
        {
            // form the service credentials cache name
            snprintf(fast_cache_name, 255, "FILE:/tmp/krb_red_fast_%d", pid);
            fast_cache_name[255] = 0;

            // set FAST cache name
            fast_cache_name_ptr = fast_cache_name;

            // do retrieve credentials using either keytab or password
            if (service_identity->has_kerberos_keytab())
            {
                LOG(LOG_INFO, "Retrieving service credentials using keytab at '%s'...",
                    service_identity->get_kerberos_keytab_path());

                ret = this->sspi_credentials->get_credentials_keytab(
                    service_identity->get_kerberos_principal_name(),
                    service_identity->get_kerberos_keytab_path(),
                    fast_cache_name, nullptr);
            }
            else
            {
                LOG(LOG_INFO, "Retrieving service credentials using password...");

                ret = this->sspi_credentials->get_credentials_password(
                    service_identity->get_kerberos_principal_name(),
                    service_identity->get_kerberos_password(),
                    fast_cache_name, nullptr);
            }
            if (ret)
            {
                LOG(LOG_ERR, "Failed to retrieve service credentials (%d)", ret);

                goto cleanup;
            }
            else
            {
                LOG(LOG_INFO, "Service credentials cached to '%s'", fast_cache_name_ptr);
            }
        }

        // form the main credentials cache name
        snprintf(cache_name, 255, "FILE:/tmp/krb_red_%d", pid);
        cache_name[255] = 0;

        // set main credentials cache name in environment
        setenv("KRB5CCNAME", cache_name, 1);
        LOG(LOG_INFO, "set KRB5CCNAME to %s", cache_name);

        // retrieve and cache main credentials
        if (identity)
        {
            LOG(LOG_INFO, "Retrieving main credentials using password...");

            // do retrieve credentials using password
            ret = this->sspi_credentials->get_credentials_password(
                identity->get_kerberos_principal_name(),
                identity->get_kerberos_password(),
                nullptr, fast_cache_name_ptr);

            if (ret)
            {
                LOG(LOG_ERR, "Failed to retrieve main credentials (%d)", ret);

                goto cleanup;
            }
            else
            {
                LOG(LOG_INFO, "Main credentials cached to '%s'", cache_name);
            }
        }

    cleanup:
        // destroy service credentials
        if (fast_cache_name_ptr)
        {
            this->sspi_credentials->destroy_credentials(fast_cache_name_ptr);
        }

        return (ret ? SEC_E_NO_CREDENTIALS : SEC_E_OK);
    }

    bool sspi_get_service_name(const std::string & server, gss_name_t * name) {
        gss_buffer_desc output;
        const char* service_name = "TERMSRV";
        gss_OID type = GSS_C_NT_HOSTBASED_SERVICE;
        auto size = (strlen(service_name) + 1 + server.size() + 1);

        auto output_value = std::make_unique<char[]>(size);
        output.value = output_value.get();
        snprintf(static_cast<char*>(output.value), size, "%s@%s", service_name, server.c_str());
        output.length = strlen(static_cast<char*>(output.value)) + 1;
        LOG(LOG_INFO, "GSS IMPORT NAME : %s", static_cast<char*>(output.value));
        OM_uint32 minor_status  = 0;
        OM_uint32 major_status = gss_import_name(&minor_status, &output, type, name);
        if (GSS_ERROR(major_status)) {
            LOG(LOG_ERR, "Failed to create service principal name");
            return false;
        }
        return true;
    }


    // GSS_Init_sec_context
    // INITIALIZE_SECURITY_CONTEXT_FN InitializeSecurityContext;

    // GSS_Accept_sec_context
    // ACCEPT_SECURITY_CONTEXT AcceptSecurityContext;
    // Not used by client

    // GSS_Wrap
    // ENCRYPT_MESSAGE EncryptMessage;
    SEC_STATUS sspi_EncryptMessage(u8_array_view data_in, std::vector<uint8_t>& data_out, unsigned long MessageSeqNo) {
        (void)MessageSeqNo;
        // OM_uint32 KRB5_CALLCONV
        // gss_wrap(
        //     OM_uint32 *,        /* minor_status */
        //     gss_ctx_id_t,       /* context_handle */
        //     int,                /* conf_req_flag */
        //     gss_qop_t,          /* qop_req */
        //     gss_buffer_t,       /* input_message_buffer */
        //     int *,              /* conf_state */
        //     gss_buffer_t);      /* output_message_buffer */

        OM_uint32 major_status;
        OM_uint32 minor_status;
        int conf_state;
        if (!this->sspi_krb_ctx) {
            return SEC_E_NO_CONTEXT;
        }
        gss_buffer_desc inbuf;
        gss_buffer_desc outbuf;

        inbuf.value = const_cast<uint8_t*>(data_in.data()); /*NOLINT*/
        inbuf.length = data_in.size();
        // LOG(LOG_INFO, "GSS_WRAP inbuf length : %d", inbuf.length);
        major_status = gss_wrap(&minor_status, this->sspi_krb_ctx->gss_ctx, true,
                GSS_C_QOP_DEFAULT, &inbuf, &conf_state, &outbuf);
        if (GSS_ERROR(major_status)) {
            this->sspi_report_error("CredSSP: GSS WRAP failed.",
                                    major_status, minor_status);
            return SEC_E_ENCRYPT_FAILURE;
        }
        // LOG(LOG_INFO, "GSS_WRAP outbuf length : %d", outbuf.length);
        data_out.assign(static_cast<uint8_t const*>(outbuf.value), static_cast<uint8_t const*>(outbuf.value)+ outbuf.length);
        gss_release_buffer(&minor_status, &outbuf);

        return SEC_E_OK;
    }

    // GSS_Unwrap
    // DECRYPT_MESSAGE DecryptMessage;
    SEC_STATUS sspi_DecryptMessage(u8_array_view data_in, std::vector<uint8_t>& data_out, unsigned long MessageSeqNo) {
        (void)MessageSeqNo;

        // OM_uint32 gss_unwrap
        //     (OM_uint32 ,             /* minor_status */
        //      const gss_ctx_id_t,     /* context_handle */
        //      const gss_buffer_t,     /* input_message_buffer */
        //      gss_buffer_t,           /* output_message_buffer */
        //      int ,                   /* conf_state */
        //      gss_qop_t *             /* qop_state */
        //      );

        int conf_state;
        gss_qop_t qop_state;
        if (!this->sspi_krb_ctx) {
            return SEC_E_NO_CONTEXT;
        }
        gss_buffer_desc inbuf;
        gss_buffer_desc outbuf;
        inbuf.value = const_cast<uint8_t*>(data_in.data()); /*NOLINT*/
        inbuf.length = data_in.size();
        // LOG(LOG_INFO, "GSS_UNWRAP inbuf length : %d", inbuf.length);
        OM_uint32 minor_status = 0;
        OM_uint32 major_status = gss_unwrap(&minor_status, this->sspi_krb_ctx->gss_ctx, &inbuf, &outbuf,
                                  &conf_state, &qop_state);
        if (GSS_ERROR(major_status)) {
            this->sspi_report_error("CredSSP: GSS UNWRAP failed.",
                                    major_status, minor_status);
            return SEC_E_DECRYPT_FAILURE;
        }
        // LOG(LOG_INFO, "GSS_UNWRAP outbuf length : %d", outbuf.length);
        data_out.assign(static_cast<uint8_t const*>(outbuf.value),static_cast<uint8_t const*>(outbuf.value)+ outbuf.length);
        gss_release_buffer(&minor_status, &outbuf);
        return SEC_E_OK;
    }

    void sspi_report_error(const char *str,
                           OM_uint32 major_status, OM_uint32 minor_status)
    {
        OM_uint32 msgctx = 0;
        OM_uint32 ms;
        gss_buffer_desc status_string;

        LOG(LOG_ERR, "GSS Major error [%u:%u:%u] %s",
            (major_status & 0xff000000) >> 24,    // Calling error
            (major_status & 0xff0000) >> 16,    // Routine error
            major_status & 0xffff,    // Supplementary info bits
            str);

        if (GSS_CALLING_ERROR(major_status)) {
            LOG(LOG_ERR, "GSS Calling error: %s",
                get_gssapi_calling_err(major_status));
        }

        if (GSS_ROUTINE_ERROR(major_status)) {
            LOG(LOG_ERR, "GSS Routine error: %s",
                get_gssapi_routine_err(major_status));
        }

        if (GSS_SUPPLEMENTARY_INFO(major_status)) {
            LOG(LOG_ERR, "GSS Supplementary info: %s",
                get_gssapi_supp_info(major_status));
        }

        LOG(LOG_ERR, "GSS Minor status error %d %s",
            static_cast<int>(minor_status),
            get_krb_err_message(minor_status));


        do {
            ms = gss_display_status(
                &minor_status, major_status,
                GSS_C_GSS_CODE, GSS_C_NULL_OID,
                &msgctx, &status_string);
            LOG(LOG_ERR," - %s", static_cast<char const*>(status_string.value));
            gss_release_buffer(&minor_status, &status_string);
            if (ms != GSS_S_COMPLETE) {
                continue;
            }
        }
        while (ms == GSS_S_COMPLETE && msgctx);

    }

    bool sspi_mech_available(gss_OID mech)
    {
        int mech_found;
        gss_OID_set mech_set;

        mech_found = 0;

        if (mech == GSS_C_NO_OID) {
            return true;
        }

        OM_uint32 minor_status = 0;
        OM_uint32 major_status = gss_indicate_mechs(&minor_status, &mech_set);
        if (!mech_set) {
            return false;
        }
        if (GSS_ERROR(major_status)) {
            this->sspi_report_error("Failed to get available mechs on system",
                                    major_status, minor_status);
            gss_release_oid_set(&minor_status, &mech_set);
            return false;
        }

        gss_test_oid_set_member(&minor_status, mech, mech_set, &mech_found);

        gss_release_oid_set(&minor_status, &mech_set);
        if (GSS_ERROR(major_status)) {
            this->sspi_report_error("Failed to match mechanism in set",
                                    major_status, minor_status);
            return false;
        }

        return mech_found != 0;
    }

    bool restricted_admin_mode;

    const std::string target_host;
    Random & rand;
    const bool credssp_verbose;
    const bool verbose;

    Transport & trans;

    void SetHostnameFromUtf8(std::string_view pszTargetName) {
        this->ServicePrincipalName = pszTargetName;
    }

    void credssp_generate_client_nonce() {
        LOG(LOG_INFO, "rdpCredsspClientKerberos::credssp generate client nonce");
        this->rand.random(writable_array_view(this->SavedClientNonce.clientNonce.data(), CLIENT_NONCE_LENGTH));
        this->SavedClientNonce.initialized = true;
        this->credssp_set_client_nonce();
    }

    void credssp_get_client_nonce() {
        LOG(LOG_INFO, "rdpCredsspClientKerberos::credssp get client nonce");
        if (this->ts_request.clientNonce.isset()){
            this->SavedClientNonce = this->ts_request.clientNonce;
        }
    }
    void credssp_set_client_nonce() {
        LOG(LOG_INFO, "rdpCredsspClientKerberos::credssp set client nonce");
        if (!this->ts_request.clientNonce.isset()) {
            this->ts_request.clientNonce = this->SavedClientNonce;
        }
    }

    void credssp_generate_public_key_hash_client_to_server() {
        LOG(LOG_INFO, "rdpCredsspClientKerberos::generate credssp public key hash (client->server)");
        SslSha256 sha256;
        uint8_t hash[SslSha256::DIGEST_LENGTH];
        sha256.update("CredSSP Client-To-Server Binding Hash\0"_av);
        sha256.update(this->SavedClientNonce.clientNonce);
        sha256.update({this->PublicKey.data(),this->PublicKey.size()});
        sha256.final(make_writable_sized_array_view(hash));

        this->ClientServerHash.assign(hash, hash+sizeof(hash));
    }

    void credssp_generate_public_key_hash_server_to_client() {
        LOG(LOG_INFO, "rdpCredsspClientKerberos::generate credssp public key hash (server->client)");
        SslSha256 sha256;
        uint8_t hash[SslSha256::DIGEST_LENGTH];
        sha256.update("CredSSP Server-To-Client Binding Hash\0"_av);
        sha256.update(make_array_view(this->SavedClientNonce.clientNonce.data(), CLIENT_NONCE_LENGTH));
        sha256.update({this->PublicKey.data(),this->PublicKey.size()});
        sha256.final(make_writable_sized_array_view(hash));
        this->ServerClientHash.assign(hash, hash + sizeof(hash));
    }

    SEC_STATUS credssp_encrypt_public_key_echo() {
        LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::encrypt_public_key_echo");
        uint32_t version = this->ts_request.use_version;

        u8_array_view public_key = {this->PublicKey.data(),this->PublicKey.size()};
        if (version >= 5) {
            this->credssp_generate_client_nonce();
            this->credssp_generate_public_key_hash_client_to_server();
            public_key = {this->ClientServerHash.data(),this->ClientServerHash.size()};
        }

        return this->sspi_EncryptMessage(public_key, this->ts_request.pubKeyAuth, this->send_seq_num++);
    }

    SEC_STATUS credssp_decrypt_public_key_echo() {
        LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::decrypt_public_key_echo");

        std::vector<uint8_t> Buffer;

        SEC_STATUS const status = this->sspi_DecryptMessage(
            this->ts_request.pubKeyAuth, Buffer, this->recv_seq_num++);

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "sspi_DecryptMessage failure: 0x%08X", status);
            return status;
        }

        const uint32_t version = this->ts_request.use_version;

        u8_array_view public_key = {this->PublicKey.data(),this->PublicKey.size()};
        if (version >= 5) {
            this->credssp_get_client_nonce();
            this->credssp_generate_public_key_hash_server_to_client();
            public_key = {this->ServerClientHash.data(), this->ServerClientHash.size()};
        }

        writable_u8_array_view public_key2 {Buffer};

        if (public_key2.size() != public_key.size()) {
            LOG(LOG_ERR, "Decrypted Pub Key length or hash length does not match ! (%zu != %zu)", public_key2.size(), public_key.size());
            return SEC_E_MESSAGE_ALTERED; /* DO NOT SEND CREDENTIALS! */
        }

        if (version < 5) {
            // if we are client and protocol is 2,3,4
            // then get the public key minus one
            ::ap_integer_decrement_le(public_key2);
        }

        if (memcmp(public_key.data(), public_key2.data(), public_key.size()) != 0) {
            LOG(LOG_ERR, "Could not verify server's public key echo");

            LOG(LOG_ERR, "Expected (length = %zu):", public_key.size());
            hexdump_c(public_key);

            LOG(LOG_ERR, "Actual (length = %zu):", public_key.size());
            hexdump_c(public_key2);

            return SEC_E_MESSAGE_ALTERED; /* DO NOT SEND CREDENTIALS! */
        }

        return SEC_E_OK;
    }

    enum class Res : bool { Err, Ok };

    struct ClientAuthenticateData
    {
        enum : uint8_t { Start, Loop, Final } state = Start;
        std::vector<uint8_t> input_buffer;
    };
    ClientAuthenticateData client_auth_data;

    Res authenticate_send()
    {
        /*
         * from tspkg.dll: 0x00000132
         * ISC_REQ_MUTUAL_AUTH
         * ISC_REQ_CONFIDENTIALITY
         * ISC_REQ_USE_SESSION_KEY
         * ISC_REQ_ALLOCATE_MEMORY
         */
        //unsigned long const fContextReq
        //  = ISC_REQ_MUTUAL_AUTH | ISC_REQ_CONFIDENTIALITY | ISC_REQ_USE_SESSION_KEY;

        u8_array_view input_buffer = this->client_auth_data.input_buffer;
        /*output*/std::vector<uint8_t> & output_buffer = this->ts_request.negoTokens;

        gss_cred_id_t gss_no_cred = GSS_C_NO_CREDENTIAL;

        // Target name (server name, ip ...)
        if (!this->sspi_get_service_name(this->ServicePrincipalName, &this->sspi_krb_ctx->target_name)) {
            // SEC_E_WRONG_PRINCIPAL;
            LOG(LOG_ERR, "Initialize Security Context Error !");
            return Res::Err;
        }
        // else {
        //     LOG(LOG_INFO, "Initialiaze Sec CTX: USE FORMER CONTEXT");
        // }

        // Token Buffer
        gss_buffer_desc input_tok;
        gss_buffer_desc output_tok;
        output_tok.length = 0;
        // LOG(LOG_INFO, "GOT INPUT BUFFER: length %d",
        //     input_buffer->Buffer.size());
        input_tok.length = input_buffer.size();
        input_tok.value = const_cast<uint8_t*>(input_buffer.data()); /*NOLINT*/

        gss_OID_desc desired_mech = gss_spnego_krb5_mechanism_oid_desc();
        if (!this->sspi_mech_available(&desired_mech)) {
            LOG(LOG_ERR, "Desired Mech unavailable");
            // SEC_E_CRYPTO_SYSTEM_INVALID;
            LOG(LOG_ERR, "Initialize Security Context Error !");
            return Res::Err;
        }
// OM_uint32 gss_init_sec_context
//                  (OM_uint32 ,             /* minor_status */
//                   const gss_cred_id_t,    /* initiator_cred_handle */
//                   gss_ctx_id_t ,          /* context_handle */
//                   const gss_name_t,       /* target_name */
//                   const gss_OID,          /* mech_type */
//                   OM_uint32,              /* req_flags */
//                   OM_uint32,              /* time_req */
//                   const gss_channel_bindings_t,
//                                           /* input_chan_bindings */
//                   const gss_buffer_t,     /* input_token */
//                   gss_OID ,               /* actual_mech_type */
//                   gss_buffer_t,           /* output_token */
//                   OM_uint32 ,             /* ret_flags */
//                   OM_uint32 *             /* time_rec */
//                  );
        OM_uint32 minor_status = 0;
        OM_uint32 major_status = gss_init_sec_context(&minor_status,
                                            gss_no_cred,
                                            &this->sspi_krb_ctx->gss_ctx,
                                            this->sspi_krb_ctx->target_name,
                                            &desired_mech,
                                            GSS_C_MUTUAL_FLAG,
                                            GSS_C_INDEFINITE,
                                            GSS_C_NO_CHANNEL_BINDINGS,
                                            &input_tok,
                                            &this->sspi_krb_ctx->actual_mech,
                                            &output_tok,
                                            &this->sspi_krb_ctx->actual_services,
                                            &this->sspi_krb_ctx->actual_time);

        if (GSS_ERROR(major_status)) {
            this->sspi_report_error("CredSSP: SPNEGO negotiation failed.",
                                    major_status, minor_status);
            // SEC_E_OUT_OF_SEQUENCE;
            LOG(LOG_ERR, "Initialize Security Context Error !");
            return Res::Err;
        }

        // LOG(LOG_INFO, "output tok length : %d", output_tok.length);
        output_buffer.assign(static_cast<uint8_t const*>(output_tok.value), static_cast<uint8_t const*>(output_tok.value)+output_tok.length);

        (void) gss_release_buffer(&minor_status, &output_tok);

        if (major_status & GSS_S_CONTINUE_NEEDED) {
            // LOG(LOG_INFO, "MAJOR CONTINUE NEEDED");
            (void) gss_release_buffer(&minor_status, &input_tok);
            this->client_auth_data.input_buffer.clear();
            if (!this->ts_request.negoTokens.empty()) {
                LOG_IF(this->verbose, LOG_INFO, "rdpCredssp - Client Authentication : Sending Authentication Token");
                LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::send");
                StaticOutStream<65536> ts_request_emit;
                auto v = emitTSRequest(this->ts_request.version,
                                       this->ts_request.negoTokens,
                                       this->ts_request.authInfo,
                                       this->ts_request.pubKeyAuth,
                                       this->ts_request.error_code,
                                       this->ts_request.clientNonce.clientNonce,
                                       this->ts_request.clientNonce.initialized,
                                       this->credssp_verbose);
                this->error_code = this->ts_request.error_code;
                ts_request_emit.out_copy_bytes(v);
                this->trans.send(ts_request_emit.get_produced_bytes());
                this->ts_request.negoTokens.clear();
                this->ts_request.pubKeyAuth.clear();
                this->ts_request.authInfo.clear();
                this->ts_request.clientNonce.reset();
            }
        }
        else {
            // LOG(LOG_INFO, "MAJOR COMPLETE NEEDED");
            SEC_STATUS encrypted = this->credssp_encrypt_public_key_echo();
            this->client_auth_data.input_buffer.clear();

            if (not this->ts_request.negoTokens.empty()) {
                LOG_IF(this->verbose, LOG_INFO, "rdpCredssp - Client Authentication : Sending Authentication Token");
                LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::send");
                StaticOutStream<65536> ts_request_emit;
                auto v = emitTSRequest(this->ts_request.version,
                                       this->ts_request.negoTokens,
                                       this->ts_request.authInfo,
                                       this->ts_request.pubKeyAuth,
                                       this->ts_request.error_code,
                                       this->ts_request.clientNonce.clientNonce,
                                       this->ts_request.clientNonce.initialized,
                                       this->credssp_verbose);
                this->error_code = this->ts_request.error_code;
                ts_request_emit.out_copy_bytes(v);
                this->trans.send(ts_request_emit.get_produced_bytes());
                this->ts_request.negoTokens.clear();
                this->ts_request.pubKeyAuth.clear();
                this->ts_request.authInfo.clear();
                this->ts_request.clientNonce.reset();
            }
            else if (encrypted == SEC_E_OK) {
                LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::send");
                StaticOutStream<65536> ts_request_emit;
                auto v = emitTSRequest(this->ts_request.version,
                                       this->ts_request.negoTokens,
                                       this->ts_request.authInfo,
                                       this->ts_request.pubKeyAuth,
                                       this->ts_request.error_code,
                                       this->ts_request.clientNonce.clientNonce,
                                       this->ts_request.clientNonce.initialized,
                                       this->credssp_verbose);
                this->error_code = this->ts_request.error_code;
                ts_request_emit.out_copy_bytes(v);
                this->trans.send(ts_request_emit.get_produced_bytes());
                this->ts_request.negoTokens.clear();
                this->ts_request.pubKeyAuth.clear();
                this->ts_request.authInfo.clear();
                this->ts_request.clientNonce.reset();
            }

            LOG_IF(this->verbose, LOG_INFO, "rdpCredssp Token loop: CONTINUE_NEEDED");
            this->client_auth_data.state = ClientAuthenticateData::Final;
            /* receive server response and place in input buffer */
        }
        return Res::Ok;
    }


    SEC_STATUS credssp_encrypt_ts_credentials() {

        LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::encrypt_ts_credentials");

        StaticOutStream<65536> ts_credentials_send;
        std::vector<uint8_t> result;
        if (this->ts_credentials.credType == 1){
            if (this->restricted_admin_mode) {
                LOG(LOG_INFO, "Restricted Admin Mode");
                result = emitTSCredentialsPassword({},std::string{}, {}, this->credssp_verbose);
            }
            else {
                result = emitTSCredentialsPassword(this->identity.get_domain_utf16(),
                    this->identity.get_username_utf16(), this->identity.get_password_utf16(),
                    this->credssp_verbose);
            }
        }
        else {
            // Card Reader Not Supported Yet
            bytes_view pin;
            bytes_view userHint;
            bytes_view domainHint;
            uint32_t keySpec = 0;
            bytes_view cardName;
            bytes_view readerName;
            bytes_view containerName;
            bytes_view cspName;
            result = emitTSCredentialsSmartCard(pin, userHint, domainHint, keySpec, cardName, readerName, containerName, cspName, this->credssp_verbose);
        }
        ts_credentials_send.out_copy_bytes(result);

        return this->sspi_EncryptMessage(
            {ts_credentials_send.get_data(), ts_credentials_send.get_offset()},
            this->ts_request.authInfo, this->send_seq_num++);
    }

public:
    rdpCredsspClientKerberos(OutTransport transport,
               std::string_view hostname,
               std::string_view target_host,
               bytes_view domain,
               std::string_view username,
               const uint8_t * password,
/*               std::string_view keytab_path,*/
               const bool restricted_admin_mode,
               std::string_view service_username,
               const uint8_t * service_password,
               std::string_view service_keytab_path,
               Random & rand,
               const bool credssp_verbose,
               const bool verbose)
        : ts_request(6) // Credssp Version 6 Supported
        , restricted_admin_mode(restricted_admin_mode)
        , target_host(target_host)
        , rand(rand)
        , credssp_verbose(credssp_verbose)
        , verbose(verbose)
        , trans(transport.get_transport())
    {
        LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::Initialization");

        this->SetHostnameFromUtf8(hostname);

        this->identity.set_username_from_utf8(username);
        this->identity.set_domain_from_utf8(domain);
        this->identity.set_password_from_utf8(password);

        this->service_identity.set_username_from_utf8(service_username);
        this->service_identity.set_domain_from_utf8(domain);
        this->service_identity.set_password_from_utf8(service_password);
        this->service_identity.set_kerberos_keytab_path(service_keytab_path);

        this->client_auth_data.state = ClientAuthenticateData::Start;

        LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::client_authenticate");
        LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::ntlm_init");

        // ============================================
        /* Get Public Key From TLS Layer and hostname */
        // ============================================

        auto const key = this->trans.get_public_key();
        this->PublicKey.assign(key.data(), key.data()+key.size());

        LOG(LOG_INFO, "Credssp: KERBEROS Authentication");

        SEC_STATUS status = this->sspi_AcquireCredentialsHandle(this->target_host, this->ServicePrincipalName,
            &this->identity, &this->service_identity);

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "Kerberos InitSecurityInterface status:%s0x%08X, fallback to NTLM",
                (status == SEC_E_NO_CREDENTIALS)?" No Credentials ":" ", status);
            throw Error(ERR_CREDSSP_KERBEROS_INIT_FAILED);
        }

        this->client_auth_data.input_buffer.clear();

        this->client_auth_data.state = ClientAuthenticateData::Loop;

        /*
         * from tspkg.dll: 0x00000132
         * ISC_REQ_MUTUAL_AUTH
         * ISC_REQ_CONFIDENTIALITY
         * ISC_REQ_USE_SESSION_KEY
         * ISC_REQ_ALLOCATE_MEMORY
         */
        //unsigned long const fContextReq

        //  = ISC_REQ_MUTUAL_AUTH | ISC_REQ_CONFIDENTIALITY | ISC_REQ_USE_SESSION_KEY;

        const std::string pszTargetName = this->ServicePrincipalName;
        std::vector<uint8_t> & output_buffer = this->ts_request.negoTokens;

        gss_cred_id_t gss_no_cred = GSS_C_NO_CREDENTIAL;
        if (!this->sspi_krb_ctx) {
            // LOG(LOG_INFO, "Initialiaze Sec Ctx: NO CONTEXT");
            this->sspi_krb_ctx = std::make_unique<KERBEROSContext>();

            // Target name (server name, ip ...)
            if (!this->sspi_get_service_name(pszTargetName, &this->sspi_krb_ctx->target_name)) {
                // SEC_E_WRONG_PRINCIPAL;
                throw Error(ERR_CREDSSP_KERBEROS_INIT_FAILED);
            }
        }

        // Token Buffer
        gss_buffer_desc input_tok;
        gss_buffer_desc output_tok;
        output_tok.length = 0;
        input_tok.length = 0;
        input_tok.value = nullptr;

        gss_OID_desc desired_mech = gss_spnego_krb5_mechanism_oid_desc();
        if (!this->sspi_mech_available(&desired_mech)) {
            LOG(LOG_ERR, "Desired Mech unavailable");
            // SEC_E_CRYPTO_SYSTEM_INVALID;
            throw Error(ERR_CREDSSP_KERBEROS_INIT_FAILED);
        }
        OM_uint32 minor_status = 0;
        OM_uint32 major_status = gss_init_sec_context(&minor_status,
                                            gss_no_cred,
                                            &this->sspi_krb_ctx->gss_ctx,
                                            this->sspi_krb_ctx->target_name,
                                            &desired_mech,
                                            GSS_C_MUTUAL_FLAG,
                                            GSS_C_INDEFINITE,
                                            GSS_C_NO_CHANNEL_BINDINGS,
                                            &input_tok,
                                            &this->sspi_krb_ctx->actual_mech,
                                            &output_tok,
                                            &this->sspi_krb_ctx->actual_services,
                                            &this->sspi_krb_ctx->actual_time);

        if (GSS_ERROR(major_status)) {
            this->sspi_report_error("CredSSP: SPNEGO negotiation failed.",
                                    major_status, minor_status);
            // SEC_E_OUT_OF_SEQUENCE;
            throw Error(ERR_CREDSSP_KERBEROS_INIT_FAILED);
        }

        // LOG(LOG_INFO, "output tok length : %d", output_tok.length);
        output_buffer.assign(static_cast<uint8_t const*>(output_tok.value), static_cast<uint8_t const*>(output_tok.value)+output_tok.length);

        (void) gss_release_buffer(&minor_status, &output_tok);

        SEC_STATUS encrypted = SEC_E_INVALID_TOKEN;
        status = SEC_E_OK;
        if (major_status & GSS_S_CONTINUE_NEEDED) {
            // LOG(LOG_INFO, "MAJOR CONTINUE NEEDED");
            (void) gss_release_buffer(&minor_status, &input_tok);
            status = SEC_I_CONTINUE_NEEDED;
        }
        else {
            // have_pub_key_auth = true;
            encrypted = this->credssp_encrypt_public_key_echo();
        }
        this->client_auth_data.input_buffer.clear();

        /* send authentication token to server */
        if (not this->ts_request.negoTokens.empty()) {
            LOG_IF(this->verbose, LOG_INFO, "rdpCredssp - Client Authentication : Sending Authentication Token");
            LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::send");
            StaticOutStream<65536> ts_request_emit;
            auto v = emitTSRequest(this->ts_request.version,
                                   this->ts_request.negoTokens,
                                   this->ts_request.authInfo,
                                   this->ts_request.pubKeyAuth,
                                   this->ts_request.error_code,
                                   this->ts_request.clientNonce.clientNonce,
                                   this->ts_request.clientNonce.initialized,
                                   this->credssp_verbose);
            this->error_code = this->ts_request.error_code;
            ts_request_emit.out_copy_bytes(v);
            this->trans.send(ts_request_emit.get_produced_bytes());
            this->ts_request.negoTokens.clear();
            this->ts_request.pubKeyAuth.clear();
            this->ts_request.authInfo.clear();
            this->ts_request.clientNonce.reset();
        }
        else if (encrypted == SEC_E_OK) {
            LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::send");
            StaticOutStream<65536> ts_request_emit;
            auto v = emitTSRequest(this->ts_request.version,
                                   this->ts_request.negoTokens,
                                   this->ts_request.authInfo,
                                   this->ts_request.pubKeyAuth,
                                   this->ts_request.error_code,
                                   this->ts_request.clientNonce.clientNonce,
                                   this->ts_request.clientNonce.initialized,
                                   this->credssp_verbose);
            this->error_code = this->ts_request.error_code;
            ts_request_emit.out_copy_bytes(v);
            this->trans.send(ts_request_emit.get_produced_bytes());
            this->ts_request.negoTokens.clear();
            this->ts_request.pubKeyAuth.clear();
            this->ts_request.authInfo.clear();
            this->ts_request.clientNonce.reset();
        }

        if (status != SEC_I_CONTINUE_NEEDED) {
            LOG_IF(this->verbose, LOG_INFO, "rdpCredssp Token loop: CONTINUE_NEEDED");

            this->client_auth_data.state = ClientAuthenticateData::Final;
        }
    }

    ~rdpCredsspClientKerberos()
    {
        this->sspi_krb_ctx.reset();
        this->sspi_credentials.reset();
        unsetenv("KRB5CCNAME");
    }

    credssp::State authenticate_next(bytes_view in_data)
    {
        switch (this->client_auth_data.state)
        {
            case ClientAuthenticateData::Start:
                return credssp::State::Err;

            case ClientAuthenticateData::Loop:
                this->ts_request = recvTSRequest(in_data, this->credssp_verbose);
                this->error_code = this->ts_request.error_code;

                LOG_IF(this->verbose, LOG_INFO, "rdpCredssp - Client Authentication : Receiving Authentication Token");
                this->client_auth_data.input_buffer = this->ts_request.negoTokens;

                if (Res::Err == this->authenticate_send()) {
                    return credssp::State::Err;
                }
                return credssp::State::Cont;

            case ClientAuthenticateData::Final:
            {
                /* Encrypted Public Key +1 */
                LOG_IF(this->verbose, LOG_INFO, "rdpCredssp - Client Authentication : Receiving Encrypted PubKey + 1");

                this->ts_request = recvTSRequest(in_data, this->credssp_verbose);

                /* Verify Server Public Key Echo */

                SEC_STATUS status = this->credssp_decrypt_public_key_echo();
                LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::buffer_free");
                this->ts_request.negoTokens.clear();
                this->ts_request.pubKeyAuth.clear();
                this->ts_request.authInfo.clear();
                this->ts_request.clientNonce.reset();

                if (status != SEC_E_OK) {
                    LOG(LOG_ERR, "Could not verify public key echo!");
                    LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::buffer_free");
                    return credssp::State::Err;
                }

                /* Send encrypted credentials */

                status = this->credssp_encrypt_ts_credentials();

                if (status != SEC_E_OK) {
                    LOG(LOG_ERR, "credssp_encrypt_ts_credentials status: 0x%08X", status);
                    return credssp::State::Err;
                }

                LOG_IF(this->verbose, LOG_INFO, "rdpCredssp - Client Authentication : Sending Credentials");

                LOG_IF(this->verbose, LOG_INFO, "rdpCredsspClientKerberos::send");
                StaticOutStream<65536> ts_request_emit;
                auto v = emitTSRequest(this->ts_request.version,
                                       this->ts_request.negoTokens,
                                       this->ts_request.authInfo,
                                       this->ts_request.pubKeyAuth,
                                       this->ts_request.error_code,
                                       this->ts_request.clientNonce.clientNonce,
                                       this->ts_request.clientNonce.initialized,
                                       this->credssp_verbose);
                this->error_code = this->ts_request.error_code;
                ts_request_emit.out_copy_bytes(v);
                this->trans.send(ts_request_emit.get_produced_bytes());
                this->ts_request.negoTokens.clear();
                this->ts_request.pubKeyAuth.clear();
                this->ts_request.authInfo.clear();
                this->ts_request.clientNonce.reset();

                this->client_auth_data.state = ClientAuthenticateData::Start;
                return credssp::State::Finish;
            }
        }

        return credssp::State::Err;
    }
};

#endif
