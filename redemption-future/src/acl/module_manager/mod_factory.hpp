/*
SPDX-FileCopyrightText: 2022 Wallix Proxies Team

SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "acl/mod_wrapper.hpp"
#include "utils/redirection_info.hpp"
#include "utils/theme.hpp"
#include "translation/translation.hpp"
#include "acl/file_system_license_store.hpp"
#include "RAIL/client_execute.hpp"
#include "configs/autogen/enums.hpp"
#include "acl/module_manager/create_module_rdp.hpp"


class SocketTransport;
class CopyPaste;


class ModFactory
{
public:
    ModFactory(EventContainer & events,
               CRef<TimeBase> time_base,
               CRef<ClientInfo> client_info,
               FrontAPI & front,
               gdi::GraphicApi & graphics,
               CRef<BGRPalette> palette,
               CRef<Font> glyphs,
               Inifile & ini,
               Keymap & keymap,
               Random & gen,
               CryptoContext & cctx,
               ErrorMessageCtx & err_msg_ctx,
               Translator translator,
               Translator log_translation
    );

    ~ModFactory();

    void set_close_box_error_id(error_type eid) noexcept
    {
        error_id = eid;
    }

    void set_translator(Translator translator) noexcept
    {
        mod_wrapper.set_translator(translator);
    }

    Translator get_translator() const noexcept
    {
        return mod_wrapper.get_translator();
    }

    zstring_view tr(TrKey k) const noexcept
    {
        return mod_wrapper.tr(k);
    }

    RedirectionInfo& get_redir_info() noexcept
    {
        return redir_info;
    }

    ModuleName mod_name() const noexcept
    {
        return current_mod;
    }

    mod_api& mod() noexcept
    {
        return mod_wrapper.get_mod();
    }

    mod_api const& mod() const noexcept
    {
        return mod_wrapper.get_mod();
    }

    SocketTransport* mod_sck_transport() const noexcept
    {
        return psocket_transport;
    }

    Callback& callback() noexcept
    {
        return mod_wrapper.get_callback();
    }

    void set_enable_osd_display_remote_target(bool enable) noexcept
    {
        mod_wrapper.set_enable_osd_display_remote_target(enable);
    }

    void display_osd_message(std::string_view message)
    {
        mod_wrapper.display_osd_message(message);
    }

    void set_time_close(MonotonicTimePoint t)
    {
        mod_wrapper.set_time_close(t);
    }

    bool is_connected() const noexcept
    {
        return this->psocket_transport;
    }

    void disconnect();

    [[nodiscard]]
    bool update_close_mod(ModuleName name);

    void create_mod_bouncer();

    void create_mod_replay();

    void create_widget_test_mod();

    void create_test_card_mod();

    void create_selector_mod();

    void create_close_mod();

    void create_close_mod_back_to_selector();

    void create_interactive_target_mod();

    void create_valid_message_mod();

    void create_display_message_mod();

    void create_dialog_challenge_mod();

    void create_display_link_mod();

    void create_wait_info_mod();

    void create_transition_mod();

    void create_login_mod();

    void create_rdp_mod(
        SessionLogApi& session_log,
        PerformAutomaticReconnection perform_automatic_reconnection
    );

    void create_vnc_mod(SessionLogApi& session_log);

private:
    class Impl;
    friend class Impl;

    SocketTransport * psocket_transport = nullptr;
    ModuleName current_mod = ModuleName::UNKNOWN;

    EventContainer & events;
    ClientInfo const& client_info;
    FrontAPI & front;
    gdi::GraphicApi & graphics;
    Inifile & ini;
    Font const & glyphs;
    Keymap & keymap;
    Random & gen;
    CryptoContext & cctx;
    ErrorMessageCtx & err_msg_ctx;
    Translator log_tr;
    error_type error_id = NO_ERROR;

    std::unique_ptr<CopyPaste> copy_paste_ptr;
    std::array<uint8_t, 28> server_auto_reconnect_packet {};
    FileSystemLicenseStore file_system_license_store;

    Theme theme;
    ClientExecute rail_client_execute;
    ModWrapper mod_wrapper;
    RedirectionInfo redir_info;
};
