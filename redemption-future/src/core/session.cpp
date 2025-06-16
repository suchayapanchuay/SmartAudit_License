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
   Copyright (C) Wallix 2010-2012
   Author(s): Christophe Grosjean, Javier Caverni, Raphael Zhou, Meng Tan
*/

#include "acl/module_manager/mod_factory.hpp"
#include "acl/module_manager/enums.hpp"
#include "core/session.hpp"
#include "core/session_events.hpp"
#include "core/session_verbose.hpp"
#include "core/pid_file.hpp"
#include "core/guest_ctx.hpp"
#include "mod/null/null.hpp"

#include "acl/acl_serializer.hpp"
#include "acl/session_logfile.hpp"
#include "acl/acl_update_session_report.hpp"

#include "configs/config.hpp"
#include "front/front.hpp"
#include "transport/socket_transport.hpp"
#include "transport/ws_transport.hpp"
#include "utils/random.hpp"
#include "utils/invalid_socket.hpp"
#include "utils/netutils.hpp"
#include "utils/select.hpp"
#include "utils/log_siem.hpp"
#include "utils/redirection_info.hpp"
#include "utils/monotonic_clock.hpp"
#include "utils/to_timeval.hpp"
#include "utils/error_message_ctx.hpp"
#include "translation/trkeys.hpp"
#include "system/urandom.hpp"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <cstdlib>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <unistd.h>


using namespace std::chrono_literals;

namespace
{

Language compute_language(Inifile const& ini, KeyLayout::KbdId kbd_id)
{
    switch (ini.get<cfg::translation::login_language>()) {
    case LoginLanguage::Auto:
        switch (kbd_id) {
        case KeyLayout::KbdId(0x0000040C):  // French (Legacy, AZERTY)
        case KeyLayout::KbdId(0x0001040C):  // French (Standard, AZERTY) (note: AFNOR layout)
        case KeyLayout::KbdId(0x0002040C):  // French (Standard, BÉPO)
        case KeyLayout::KbdId(0x00001009):  // Canadian French
        case KeyLayout::KbdId(0x00000C0C):  // Canadian French (Legacy)
        case KeyLayout::KbdId(0x0000080C):  // French (Belgium)
        case KeyLayout::KbdId(0x0001080C):  // French (Belgium) Belgian (Comma)
        case KeyLayout::KbdId(0x0000100C):  // French (Switzerland)
            return Language::fr;
        }
        break;
    case LoginLanguage::EN: return Language::en;
    case LoginLanguage::FR: return Language::fr;
    }
    return ini.get<cfg::translation::language>();
}

struct FinalSocketTransport final : ::SocketTransport
{
    using ::SocketTransport::SocketTransport;
};

class Session
{
#define X_DECL_NAME(f) \
    f(capture)         \
    f(session)         \
    f(front)           \
    f(mod_rdp)         \
    f(mod_vnc)         \
    f(mod_internal)    \
    f(sck_mod)         \
    f(sck_front)       \
    f(password)        \
    f(cache)           \
    f(mod_rdp_use_failure_simulation_socket_transport)

#define DECL_DEBUG_VAR(name) cfg::debug::name::type name;
#define DEBUG_SAVE_AND_RESET(name) \
    name = ini.get<cfg::debug::name>(); \
    ini.set<cfg::debug::name>(cfg::debug::name::type{});
#define DEBUG_RESTORE(name) ini.set<cfg::debug::name>(name);

    struct DebugData
    {
        void save_and_reset_ini(Inifile& ini, std::string_view primary_user)
        {
            std::string_view s = ini.get<cfg::debug::primary_user>();
            if (s.empty() || s == primary_user) {
                return ;
            }

            X_DECL_NAME(DEBUG_SAVE_AND_RESET)
            is_restored = false;
        }

        void restore(Inifile& ini, std::string_view primary_user)
        {
            if (is_restored) {
                return ;
            }

            std::string_view s = ini.get<cfg::debug::primary_user>();
            if (s.empty() || s != primary_user) {
                return ;
            }

            X_DECL_NAME(DEBUG_RESTORE)
            is_restored = true;
        }

    private:
        bool is_restored = true;
        X_DECL_NAME(DECL_DEBUG_VAR)
    };

#undef DEBUG_RESTORE
#undef DEBUG_SAVE_AND_RESET
#undef DECL_DEBUG_VAR

#undef X_DECL_NAME

    struct AclReporter final : AclReportApi
    {
        AclReporter(Inifile& ini) : ini(ini) {}

        void acl_report(AclReport report) override
        {
            chars_view target_device = this->ini.get<cfg::globals::target_device>();
            bool concat = this->ini.is_ready_to_send<cfg::context::reporting>();
            this->ini.update_acl<cfg::context::reporting>([&](std::string& s){
                acl_update_session_report.update_report(
                    OutParam{s}, !concat, report, target_device
                );
            });
        }

        Inifile& ini;
    private:
        AclUpdateSessionReport acl_update_session_report;
    };

    struct SecondarySession final : private SessionLogApi
    {
        enum class Type { RDP, VNC, };

        SecondarySession(
            AclReporter& acl_reporter,
            CryptoContext& cctx,
            Random& rnd,
            gdi::CaptureProbeApi& probe_api,
            TimeBase const& time_base,
            bool use_debug_format)
        : acl_reporter(acl_reporter)
        , probe_api(probe_api)
        , time_base(time_base)
        , cctx(cctx)
        , log_file(
            cctx, rnd,
            SessionLogFile::Debug(use_debug_format),
            [this](Error const& error){
                if (error.errnum == ENOSPC) {
                    // error.id = ERR_TRANSPORT_WRITE_NO_ROOM;
                    this->acl_reporter.acl_report(AclReport::file_system_full());
                }
            })
        {
            auto has_drive = bool(acl_reporter.ini.get<cfg::capture::disable_file_system_log>() & FileSystemLogFlags::wrm);
            auto has_clipboard = bool(acl_reporter.ini.get<cfg::capture::disable_clipboard_log>() & ClipboardLogFlags::wrm);

            this->dont_log |= (has_drive ? LogCategoryId::Drive : LogCategoryId::None);
            this->dont_log |= (has_clipboard ? LogCategoryId::Clipboard : LogCategoryId::None);
        }

        [[nodiscard]]
        SessionLogApi& open_secondary_session(Type session_type)
        {
            assert(!this->is_open());

            switch (session_type)
            {
                case Type::RDP: this->session_type = "RDP"; break;
                case Type::VNC: this->session_type = "VNC"; break;
            }

            this->cctx.set_master_key(this->acl_reporter.ini.get<cfg::crypto::encryption_key>());
            this->cctx.set_hmac_key(this->acl_reporter.ini.get<cfg::crypto::sign_key>());
            this->cctx.set_trace_type(this->acl_reporter.ini.get<cfg::globals::trace_type>());

            auto const& subdir = this->acl_reporter.ini.get<cfg::capture::record_subdirectory>();
            auto const& record_dir = this->acl_reporter.ini.get<cfg::capture::record_path>();
            auto const& hash_dir = this->acl_reporter.ini.get<cfg::capture::hash_path>();
            auto const& filebase = this->acl_reporter.ini.get<cfg::capture::record_filebase>();

            std::string record_path = str_concat(record_dir.as_string(), subdir, '/');
            std::string hash_path = str_concat(hash_dir.as_string(), subdir, '/');

            for (auto* s : {&record_path, &hash_path}) {
                if (recursive_create_directory(s->c_str(), S_IRWXU | S_IRGRP | S_IXGRP) != 0) {
                    LOG(LOG_ERR,
                        "Session::open_secondary_session: Failed to create directory: \"%s\"", *s);
                }
            }

            std::string basename = str_concat(filebase, ".log");
            record_path += basename;
            hash_path += basename;

            this->log_file.open_session_log(
                acl_reporter.ini.get<cfg::session_log::enable_syslog_format>(),
                SessionLogFile::SaveToFile(acl_reporter.ini.get<cfg::session_log::enable_session_log_file>()),
                record_path.c_str(), hash_path.c_str(),
                acl_reporter.ini.get<cfg::capture::file_permissions>(), /*derivator=*/basename);

            return *this;
        }

        void close_secondary_session()
        {
            assert(this->is_open());
            this->session_type = {};
            this->log_file.close_session_log();
        }

        [[nodiscard]]
        SessionLogApi& get_secondary_session_log()
        {
            assert(this->is_open());
            return *this;
        }

        void acl_report(AclReport report) override
        {
            acl_reporter.acl_report(report);
        }

    private:
        #ifndef NDEBUG
        bool is_open() const
        {
            return !this->session_type.empty();
        }
        #endif

        void log6(LogId id, KVLogList kv_list) override
        {
            timespec tp;
            clock_gettime(CLOCK_REALTIME, &tp);

            this->log_file.log(tp.tv_sec, this->acl_reporter.ini, this->session_type, id, kv_list);

            if (!this->dont_log.test(detail::log_id_category_map[underlying_cast(id)])) {
                this->probe_api.session_update(this->time_base.monotonic_time, id, kv_list);
            }
        }

        void set_control_owner_ctx(chars_view name) override
        {
            this->log_file.set_control_owner_ctx(name);
        }

        AclReporter& acl_reporter;
        gdi::CaptureProbeApi& probe_api;
        TimeBase const& time_base;
        CryptoContext & cctx;
        std::string_view session_type;
        LogCategoryFlags dont_log {};
        SessionLogFile log_file;
        std::string sharing_ctx_extra_log;
    };

    struct Select
    {
        Select()
        {
            io_fd_zero(this->rfds);
        }

        int select(timeval* delay)
        {
            return ::select(
                max + 1,
                &rfds,
                want_write ? &wfds : nullptr,
                nullptr, delay);
        }

        bool is_set_for_writing(int fd) const
        {
            assert(this->want_write);
            return io_fd_isset(fd, this->wfds);
        }

        bool is_set_for_reading(int fd) const
        {
            bool ret = io_fd_isset(fd, this->rfds);
            return ret;
        }

        void set_read_sck(int fd)
        {
            assert(fd != INVALID_SOCKET);
            io_fd_set(fd, this->rfds);
            this->max = std::max(this->max, fd);
        }

        void set_write_sck(int fd)
        {
            assert(fd != INVALID_SOCKET);
            if (!this->want_write) {
                this->want_write = true;
                io_fd_zero(this->wfds);
            }

            io_fd_set(fd, this->wfds);
            this->max = std::max(this->max, fd);
        }

    private:
        int max = 0;
        bool want_write = false;
        fd_set rfds;
        fd_set wfds;
    };

    struct SessionFront final : Front
    {
        MonotonicTimePoint target_connection_start_time {};
        Inifile& ini;

        SessionFront(
            EventContainer& events,
            AclReportApi& acl_report,
            Transport& trans,
            Random& gen,
            Inifile& ini,
            CryptoContext& cctx
        )
        : Front(events, acl_report, trans, gen, ini, cctx)
        , ini(ini)
        {}

        // secondary session is ready, set target_connection_time
        bool can_be_start_capture(SessionLogApi& session_log) override
        {
            if (this->target_connection_start_time != MonotonicTimePoint{}) {
                auto elapsed = MonotonicTimePoint::clock::now()
                                - this->target_connection_start_time;
                this->ini.set_acl<cfg::globals::target_connection_time>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed));
                this->target_connection_start_time = {};
            }

            this->ini.set_acl<cfg::context::sharing_ready>(true);

            return this->Front::can_be_start_capture(session_log);
        }
    };

    Inifile & ini;
    PidFile & pid_file;
    SessionVerbose verbose;
    AclUpdateSessionReport acl_update_session_report;
    DebugData debug_data;

private:
    enum class EndSessionResult
    {
        close_box,
        retry,
        reconnection,
        redirection,
    };

    EndSessionResult end_session_exception(Error const& e, Inifile & ini, ModFactory const& mod_factory)
    {
        if (e.id == ERR_SESSION_PROBE_LAUNCH
         || e.id == ERR_SESSION_PROBE_ASBL_FSVC_UNAVAILABLE
         || e.id == ERR_SESSION_PROBE_ASBL_MAYBE_SOMETHING_BLOCKS
         || e.id == ERR_SESSION_PROBE_ASBL_UNKNOWN_REASON
         || e.id == ERR_SESSION_PROBE_CBBL_FSVC_UNAVAILABLE
         || e.id == ERR_SESSION_PROBE_CBBL_CBVC_UNAVAILABLE
         || e.id == ERR_SESSION_PROBE_CBBL_DRIVE_NOT_READY_YET
         || e.id == ERR_SESSION_PROBE_CBBL_MAYBE_SOMETHING_BLOCKS
         || e.id == ERR_SESSION_PROBE_CBBL_LAUNCH_CYCLE_INTERRUPTED
         || e.id == ERR_SESSION_PROBE_CBBL_UNKNOWN_REASON_REFER_TO_SYSLOG
         || e.id == ERR_SESSION_PROBE_RP_LAUNCH_REFER_TO_SYSLOG
        ) {
            if (ini.get<cfg::session_probe::on_launch_failure>() ==
                    SessionProbeOnLaunchFailure::retry_without_session_probe)
            {
                LOG(LOG_INFO, "Retry connection without session probe");
                ini.set<cfg::session_probe::enable_session_probe>(false);
                return EndSessionResult::retry;
            }
            return EndSessionResult::close_box;
        }

        if (e.id == ERR_SESSION_PROBE_DISCONNECTION_RECONNECTION) {
            LOG(LOG_INFO, "Retry Session Probe Disconnection Reconnection");
            return EndSessionResult::retry;
        }

        if (e.id == ERR_RAIL_RESIZING_REQUIRED) {
            return EndSessionResult::retry;
        }

        if (e.id == ERR_AUTOMATIC_RECONNECTION_REQUIRED) {
            LOG(LOG_INFO, "Retry Automatic Reconnection Required");
            return EndSessionResult::reconnection;
        }

        if (e.id == ERR_RAIL_NOT_ENABLED) {
            LOG(LOG_INFO, "Retry without native remoteapp capability");
            ini.set<cfg::mod_rdp::use_native_remoteapp_capability>(false);
            return EndSessionResult::retry;
        }

        if (e.id == ERR_RDP_SERVER_REDIR){
            if (ini.get<cfg::mod_rdp::server_redirection_support>()) {
                LOG(LOG_INFO, "Server redirection");
                return EndSessionResult::redirection;
            }
            else {
                LOG(LOG_ERR, "Start Session Failed: forbidden redirection = %s", e.errmsg());
                return EndSessionResult::close_box;
            }
        }

        if (e.id == ERR_SESSION_CLOSE_ENDDATE_REACHED){
            LOG(LOG_INFO, "Close because disconnection time reached");
            return EndSessionResult::close_box;
        }

        if (e.id == ERR_MCS_APPID_IS_MCS_DPUM){
            LOG(LOG_INFO, "Remote Session Closed by User");
            return EndSessionResult::close_box;
        }

        if (e.id == ERR_SESSION_CLOSE_ACL_KEEPALIVE_MISSED) {
            LOG(LOG_INFO, "Close because of missed ACL keepalive");
            return EndSessionResult::close_box;
        }

        if (e.id == ERR_SESSION_CLOSE_USER_INACTIVITY) {
            LOG(LOG_INFO, "Close because of user Inactivity");
            return EndSessionResult::close_box;
        }

        if ((e.id == ERR_TRANSPORT_WRITE_FAILED || e.id == ERR_TRANSPORT_NO_MORE_DATA)
         && mod_factory.mod_sck_transport()
         && mod_factory.mod_sck_transport()->get_fd() == e.data
         && ini.get<cfg::mod_rdp::auto_reconnection_on_losing_target_link>()
         && mod_factory.mod().is_auto_reconnectable()
         && !mod_factory.mod().server_error_encountered()
        ) {
            LOG(LOG_INFO, "Session::end_session_exception: target link exception. %s",
                ERR_TRANSPORT_WRITE_FAILED == e.id
                    ? "ERR_TRANSPORT_WRITE_FAILED"
                    : "ERR_TRANSPORT_NO_MORE_DATA");
            return EndSessionResult::reconnection;
        }

        LOG(LOG_INFO,
            "ModTrans=<%p> Sock=%d AutoReconnection=%s AutoReconnectable=%s ErrorEncountered=%s",
            mod_factory.mod_sck_transport(),
            (mod_factory.mod_sck_transport() ? mod_factory.mod_sck_transport()->get_fd() : -1),
            (ini.get<cfg::mod_rdp::auto_reconnection_on_losing_target_link>() ? "Yes" : "No"),
            (mod_factory.mod().is_auto_reconnectable() ? "Yes" : "No"),
            (mod_factory.mod().server_error_encountered() ? "Yes" : "No")
            );

        return EndSessionResult::close_box;
    }

private:
    static bool is_target_module(ModuleName mod)
    {
        return mod == ModuleName::RDP
            || mod == ModuleName::VNC;
    }

    static bool is_close_module(ModuleName mod)
    {
        return mod == ModuleName::close
            || mod == ModuleName::close_back;
    }

    static void set_inactivity_timeout(Inactivity& inactivity, Inifile& ini)
    {
        if (is_target_module(ini.get<cfg::context::module>())) {
            auto const& inactivity_timeout
                = ini.get<cfg::globals::inactivity_timeout>();

            auto timeout = (inactivity_timeout != inactivity_timeout.zero())
                ? inactivity_timeout
                : ini.get<cfg::globals::base_inactivity_timeout>();

            inactivity.start(timeout);
        }
    }

    std::string session_type;

    void next_backend_module(
        ModuleName next_mod,
        SecondarySession& secondary_session,
        ErrorMessageCtx& err_msg_ctx,
        ModFactory& mod_factory,
        Inactivity& inactivity,
        KeepAlive& keepalive,
        SessionFront& front,
        GuestCtx& guest_ctx,
        EventManager& event_manager)
    {
        LOG_IF(bool(this->verbose & SessionVerbose::Trace),
            LOG_INFO, "Current Mod is %s ; Next is %s",
            get_module_name(mod_factory.mod_name()),
            get_module_name(next_mod)
        );

        if (mod_factory.is_connected()) {
            LOG(LOG_INFO, "Exited from target connection");
            guest_ctx.stop();
            mod_factory.disconnect();
            if (!ini.get<cfg::context::has_more_target>()) {
                front.must_be_stop_capture();
                secondary_session.close_secondary_session();
            }
            else {
                LOG(LOG_INFO, "Keep existing capture & session log.");
            }
        }
        else if (mod_factory.update_close_mod(next_mod)) {
            return;
        }
        else {
            mod_factory.disconnect();
        }

        if (is_target_module(next_mod)) {
            keepalive.start();
            event_manager.set_time_base(TimeBase::now());
            front.target_connection_start_time = event_manager.get_monotonic_time();
        }
        else {
            keepalive.stop();
            front.target_connection_start_time = MonotonicTimePoint();
        }

        if (next_mod == ModuleName::INTERNAL) {
            next_mod = get_internal_module_id_from_target(
                this->ini.get<cfg::context::target_host>()
            );
        }

        LOG(LOG_INFO, "New Module: %s", get_module_name(next_mod));

        auto open_secondary_session = [&](SecondarySession::Type secondary_session_type){
            log_siem::set_user(this->ini.get<cfg::globals::auth_user>());
            try {
                debug_data.restore(ini, ini.get<cfg::globals::auth_user>());
                switch (secondary_session_type)
                {
                    case SecondarySession::Type::RDP:
                    {
                        SessionLogApi& session_log_api =
                              this->ini.get<cfg::context::try_alternate_target>()
                            ? secondary_session.get_secondary_session_log()
                            : secondary_session.open_secondary_session(secondary_session_type);

                        mod_factory.create_rdp_mod(
                            session_log_api,
                            PerformAutomaticReconnection::No);
                        break;
                    }
                    case SecondarySession::Type::VNC:
                        mod_factory.create_vnc_mod(
                            secondary_session.open_secondary_session(secondary_session_type));
                        break;
                }
                err_msg_ctx.clear();
                set_inactivity_timeout(inactivity, ini);
                return;
            }
            catch (Error const& /*error*/) {
                this->secondary_session_creation_failed(secondary_session, this->ini.get<cfg::context::has_more_target>());
                mod_factory.create_transition_mod();
            }
            catch (...) {
                this->secondary_session_creation_failed(secondary_session, this->ini.get<cfg::context::has_more_target>());
                throw;
            }
        };

        switch (next_mod)
        {
        case ModuleName::RDP:
            open_secondary_session(SecondarySession::Type::RDP);
            break;

        case ModuleName::VNC:
            open_secondary_session(SecondarySession::Type::VNC);
            break;

        case ModuleName::close:
            mod_factory.create_close_mod();
            inactivity.stop();
            break;

        case ModuleName::close_back:
            mod_factory.create_close_mod_back_to_selector();
            inactivity.stop();
            break;

        case ModuleName::login:
            log_siem::set_user("");
            inactivity.stop();
            mod_factory.create_login_mod();
            break;

        case ModuleName::waitinfo:
            log_siem::set_user("");
            inactivity.stop();
            mod_factory.create_wait_info_mod();
            break;

        case ModuleName::confirm:
            log_siem::set_user("");
            inactivity.start(this->ini.get<cfg::globals::base_inactivity_timeout>());
            mod_factory.create_display_message_mod();
            break;

        case ModuleName::link_confirm:
            log_siem::set_user("");
            if (auto timeout = this->ini.get<cfg::context::mod_timeout>()
                ; timeout.count() != 0
            ) {
                inactivity.start(timeout);
            }
            else {
                inactivity.start(this->ini.get<cfg::globals::base_inactivity_timeout>());
            }
            mod_factory.create_display_link_mod();
            break;

        case ModuleName::valid:
            log_siem::set_user("");
            inactivity.start(this->ini.get<cfg::globals::base_inactivity_timeout>());
            mod_factory.create_valid_message_mod();
            break;

        case ModuleName::challenge:
            log_siem::set_user("");
            inactivity.start(this->ini.get<cfg::globals::base_inactivity_timeout>());
            mod_factory.create_dialog_challenge_mod();
            break;

        case ModuleName::selector:
            inactivity.start(this->ini.get<cfg::globals::base_inactivity_timeout>());
            log_siem::set_user(this->ini.get<cfg::globals::auth_user>());
            mod_factory.create_selector_mod();
            break;

        case ModuleName::bouncer2:
            log_siem::set_user(this->ini.get<cfg::globals::auth_user>());
            mod_factory.create_mod_bouncer();
            break;

        case ModuleName::autotest:
            inactivity.stop();
            log_siem::set_user(this->ini.get<cfg::globals::auth_user>());
            mod_factory.create_mod_replay();
            break;

        case ModuleName::widgettest:
            log_siem::set_user(this->ini.get<cfg::globals::auth_user>());
            mod_factory.create_widget_test_mod();
            break;

        case ModuleName::card:
            log_siem::set_user(this->ini.get<cfg::globals::auth_user>());
            mod_factory.create_test_card_mod();
            break;

        case ModuleName::interactive_target:
            log_siem::set_user(this->ini.get<cfg::globals::auth_user>());
            inactivity.start(this->ini.get<cfg::globals::base_inactivity_timeout>());
            mod_factory.create_interactive_target_mod();
            break;

        case ModuleName::transitory:
            log_siem::set_user(this->ini.get<cfg::globals::auth_user>());
            mod_factory.create_transition_mod();
            break;

        case ModuleName::INTERNAL:
        case ModuleName::UNKNOWN:
            LOG(LOG_INFO, "ModuleManager::Unknown backend exception %u", unsigned(next_mod));
            throw Error(ERR_SESSION_UNKNOWN_BACKEND);
        }
    }

    void secondary_session_creation_failed(SecondarySession & secondary_session, bool has_other_target_to_try)
    {
        secondary_session.get_secondary_session_log().log6(LogId::SESSION_CREATION_FAILED, {});
        if (!has_other_target_to_try) {
            secondary_session.close_secondary_session();
        }
        else {
            LOG(LOG_INFO, "Keep existing session log.");
        }
        this->ini.set_acl<cfg::context::module>(ModuleName::close);
    }

    bool retry_rdp(
        SecondarySession & secondary_session, ErrorMessageCtx & err_msg_ctx,
        ModFactory & mod_factory, SessionFront & front, EventManager& event_manager,
        PerformAutomaticReconnection perform_automatic_reconnection)
    {
        LOG(LOG_INFO, "Retry RDP");

        log_siem::set_user(this->ini.get<cfg::globals::auth_user>());

        mod_factory.disconnect();

        size_t const session_reconnection_delay_ms = this->ini.get<cfg::mod_rdp::session_reconnection_delay>().count();
        if (session_reconnection_delay_ms)
        {
            LOG(LOG_INFO, "Waiting for %lu ms before retrying RDP", session_reconnection_delay_ms);
            ::usleep(session_reconnection_delay_ms * 1000);
            event_manager.set_time_base(TimeBase::now());
        }

        SessionLogApi& session_log = secondary_session.get_secondary_session_log();
        try {
            front.target_connection_start_time = MonotonicTimePoint::clock::now();
            mod_factory.create_rdp_mod(session_log, perform_automatic_reconnection);
            err_msg_ctx.clear();
            return true;
        }
        catch (Error const& /*error*/) {
            front.must_be_stop_capture();
            this->secondary_session_creation_failed(secondary_session, false);
            mod_factory.create_transition_mod();
        }
        catch (...) {
            front.must_be_stop_capture();
            this->secondary_session_creation_failed(secondary_session, false);
        }

        return false;
    }

    struct NextDelay
    {
        NextDelay(bool nodelay, EventManager& event_manager)
        {
            if (!nodelay) {
                event_manager.get_writable_time_base().monotonic_time
                    = MonotonicTimePoint::clock::now();
                auto timeout = event_manager.next_timeout();
                // 0 means no timeout to trigger
                if (timeout != MonotonicTimePoint{}) {
                    auto now = MonotonicTimePoint::clock::now();
                    this->tv = (now < timeout)
                        ? to_timeval(timeout - now)
                        // no delay
                        : timeval{};
                    this->r = &this->tv;
                }
            }
            else {
                this->tv = {0, 0};
                this->r = &this->tv;
            }
        }

        timeval* timeout() const { return this->r; }

    private:
        timeval tv;
        timeval* r = nullptr;
    };

    template<class Fn>
    bool internal_front_loop(
        SessionFront& front,
        SocketTransport& front_trans,
        EventManager& event_manager,
        Callback& callback,
        Fn&& stop_event)
    {
        const int sck = front_trans.get_fd();
        const int max = sck;

        fd_set fds;
        io_fd_zero(fds);

        for (;;) {
            bool const has_data_to_write = front_trans.has_data_to_write();
            bool const has_tls_pending_data = front_trans.has_tls_pending_data();

            io_fd_set(sck, fds);

            int const num = ::select(max+1,
                has_data_to_write ? nullptr : &fds,
                has_data_to_write ? &fds : nullptr,
                nullptr, NextDelay(has_tls_pending_data, event_manager).timeout());

            if (num < 0) {
                if (errno != EINTR) {
                    return false;
                }
                continue;
            }

            event_manager.set_time_base(TimeBase::now());
            event_manager.execute_events(
                []([[maybe_unused]] int fd){ assert(fd == -1); return false; },
                bool(this->verbose & SessionVerbose::Event));
            front.sync();

            if (num) {
                assert(io_fd_isset(sck, fds));

                if (has_data_to_write) {
                    front_trans.send_waiting_data();
                }
                else {
                    front.incoming(callback);
                }
            }
            else if (has_tls_pending_data) {
                front.incoming(callback);
            }

            front.sync();

            if (stop_event()) {
                return true;
            }
        }
    }

    void acl_auth_info(ClientInfo const& client_info)
    {
        auto const kbd_id = client_info.keylayout;
        LOG_IF(bool(this->verbose & SessionVerbose::Log), LOG_INFO,
            "Session: Keyboard Layout = 0x%x", kbd_id);
        this->ini.set_acl<cfg::translation::language>(compute_language(this->ini, kbd_id));

        this->ini.set_acl<cfg::context::opt_width>(client_info.screen_info.width);
        this->ini.set_acl<cfg::context::opt_height>(client_info.screen_info.height);
        this->ini.set_acl<cfg::context::opt_bpp>(safe_int(client_info.screen_info.bpp));

        std::string username = client_info.username;
        std::string_view domain = client_info.domain;
        std::string_view password = client_info.password;
        if (not domain.empty()
         && username.find('@') == std::string::npos
         && username.find('\\') == std::string::npos
        ) {
            str_append(username, '@', domain);
        }

        LOG_IF(bool(this->verbose & SessionVerbose::Trace), LOG_INFO,
            "Session::flush_acl_auth_info(auth_user=%s)", username);

        this->ini.set_acl<cfg::globals::auth_user>(username);
        this->ini.ask<cfg::context::selector>();
        this->ini.ask<cfg::globals::target_user>();
        this->ini.ask<cfg::globals::target_device>();
        this->ini.ask<cfg::context::target_protocol>();
        if (!password.empty()) {
            this->ini.set_acl<cfg::context::password>(password);
        }
    }

    enum class EndLoopState
    {
        ImmediateDisconnection,
        ShowCloseBox
    };

    inline EndLoopState main_loop(
        int auth_sck,
        AclReporter& acl_reporter,
        EventManager& event_manager,
        ErrorMessageCtx& err_msg_ctx,
        CryptoContext& cctx,
        URandom& rnd,
        SocketTransport& front_trans,
        SessionFront& front,
        GuestCtx& guest_ctx,
        ModFactory& mod_factory,
        TranslationCatalogsRef translation_catalogs
    )
    {
        LOG_IF(bool(this->verbose & SessionVerbose::Trace),
            LOG_INFO, "Session: Main loop");

        assert(auth_sck != INVALID_SOCKET);

        FinalSocketTransport auth_trans(
            "Authentifier"_sck_name, unique_fd(auth_sck),
            ini.get<cfg::globals::authfile>(), 0,
            std::chrono::milliseconds(1000),
            std::chrono::milliseconds::zero(),
            std::chrono::milliseconds(1000),
            SocketTransport::Verbose::none);

        auto& events = event_manager.get_events();
        EndSessionWarning end_session_warning(events);

        KeepAlive keepalive(ini, events, ini.get<cfg::globals::keepalive_grace_delay>());
        Inactivity inactivity(events);

        AclSerializer acl_serial(ini, auth_trans);

        SecondarySession secondary_session(
            acl_reporter, cctx, rnd, front, event_manager.get_time_base(),
            bool(this->verbose & SessionVerbose::Log));

        enum class LoopState
        {
            AclSend,
            Select,
            Front,
            AclReceive,
            AclUpdate,
            EventLoop,
            BackEvent,
            NextMod,
            UpdateOsd,
        };

        ModuleName next_module = ModuleName::UNKNOWN;

        for (;;) {
            assert(front_trans.get_fd() != INVALID_SOCKET);
            assert(auth_trans.get_fd() != INVALID_SOCKET);

            auto loop_state = LoopState::Front;

            try {
                front.sync();

                loop_state = LoopState::AclSend;

                acl_serial.send_acl_data();

                loop_state = LoopState::Select;

                auto* pmod_trans = mod_factory.mod_sck_transport();
                const bool front_has_data_to_write     = front_trans.has_data_to_write();
                const bool front_has_tls_pending_data  = front_trans.has_tls_pending_data();
                const bool front2_has_data_to_write    = guest_ctx.has_front()
                                                      && guest_ctx.front_transport().has_data_to_write();
                const bool front2_has_tls_pending_data = guest_ctx.has_front()
                                                      && guest_ctx.front_transport().has_tls_pending_data();
                const bool mod_trans_is_valid          = pmod_trans
                                                      && pmod_trans->get_fd() != INVALID_SOCKET;
                const bool mod_has_data_to_write       = mod_trans_is_valid
                                                      && pmod_trans->has_data_to_write();
                const bool mod_has_tls_pending_data    = mod_trans_is_valid
                                                      && pmod_trans->has_tls_pending_data();

                Select ioswitch;

                // writing pending, do not read anymore (excepted acl)
                if (REDEMPTION_UNLIKELY(mod_has_data_to_write
                                     || front_has_data_to_write
                                     || front2_has_data_to_write)
                ) {
                    if (front_has_data_to_write) {
                        LOG_IF(bool(this->verbose & SessionVerbose::Trace),
                            LOG_INFO, "Session: Front has data to write");
                        ioswitch.set_write_sck(front_trans.get_fd());
                    }

                    if (front2_has_data_to_write) {
                        LOG_IF(bool(this->verbose & SessionVerbose::Trace),
                            LOG_INFO, "Session: Front2 has data to write");
                        ioswitch.set_write_sck(guest_ctx.front_transport().get_fd());
                    }

                    if (mod_has_data_to_write) {
                        LOG_IF(bool(this->verbose & SessionVerbose::Trace),
                            LOG_INFO, "Session: Mod has data to write");
                        ioswitch.set_write_sck(pmod_trans->get_fd());
                    }
                }
                else {
                    ioswitch.set_read_sck(front_trans.get_fd());
                    event_manager.for_each_fd([&](int fd){ ioswitch.set_read_sck(fd); });
                }
                ioswitch.set_read_sck(auth_sck);

                if (ioswitch.select(NextDelay(
                    mod_has_tls_pending_data || front_has_tls_pending_data || front2_has_tls_pending_data,
                    event_manager
                ).timeout()) < 0) {
                    if (errno != EINTR) {
                        // Cope with EBADF, EINVAL, ENOMEM : none of these should ever happen
                        // EBADF: means fd has been closed (by me) or as already returned an error on another call
                        // EINVAL: invalid value in timeout (my fault again)
                        // ENOMEM: no enough memory in kernel (unlikely fort 3 sockets)
                        LOG(LOG_ERR, "Proxy data wait loop raised error %d: %s",
                            errno, strerror(errno));
                        break;
                    }
                    continue;
                }

                if (front2_has_tls_pending_data) {
                    LOG_IF(bool(this->verbose & SessionVerbose::Trace),
                        LOG_INFO, "Session: Front2 has tls pending data");
                    ioswitch.set_read_sck(guest_ctx.front_transport().get_fd());
                }

                if (front_has_tls_pending_data) {
                    LOG_IF(bool(this->verbose & SessionVerbose::Trace),
                        LOG_INFO, "Session: Front has tls pending data");
                    ioswitch.set_read_sck(front_trans.get_fd());
                }

                if (mod_has_tls_pending_data) {
                    LOG_IF(bool(this->verbose & SessionVerbose::Trace),
                        LOG_INFO, "Session: Mod has tls pending data");
                    ioswitch.set_read_sck(pmod_trans->get_fd());
                }

                // update times and synchronize real time
                {
                    auto old_time_base = event_manager.get_time_base();
                    auto new_time_base = TimeBase::now();

                    event_manager.set_time_base(new_time_base);

                    constexpr auto max_delay = MonotonicTimePoint::duration(1s);
                    MonotonicTimePoint::duration monotonic_elpased
                        = new_time_base.monotonic_time - old_time_base.monotonic_time;
                    MonotonicTimePoint::duration real_elpased
                        = new_time_base.real_time - old_time_base.real_time;

                    if (abs(real_elpased) >= monotonic_elpased + max_delay) {
                        front.must_synchronize_times_capture(new_time_base.monotonic_time, new_time_base.real_time);
                        end_session_warning.add_delay(real_elpased);
                    }
                }

                loop_state = LoopState::Front;

                if (REDEMPTION_UNLIKELY(front_has_data_to_write || front2_has_data_to_write)) {
                    if (front_has_data_to_write) {
                        if (ioswitch.is_set_for_writing(front_trans.get_fd())) {
                            front_trans.send_waiting_data();
                        }
                    }

                    if (front2_has_data_to_write) {
                        if (ioswitch.is_set_for_writing(guest_ctx.front_transport().get_fd())) {
                            front_trans.send_waiting_data();
                        }
                    }
                }
                else if (ioswitch.is_set_for_reading(front_trans.get_fd())) {
                    auto& callback = mod_factory.callback();
                    front.incoming(callback);

                    // TODO should be replaced by callback.rdp_gdi_up/down() when is_up_and_running changes
                    if (front.front_must_notify_resize) {
                        LOG(LOG_INFO, "Notify resize to front");
                        front.notify_resize(callback);
                    }
                }


                loop_state = LoopState::AclReceive;

                BackEvent_t back_event = BACK_EVENT_NONE;

                if (ioswitch.is_set_for_reading(auth_sck)) {
                    mod_factory.set_enable_osd_display_remote_target(
                        ini.get<cfg::globals::enable_osd_display_remote_target>()
                    );

                    AclFieldMask updated_fields = acl_serial.incoming();

                    loop_state = LoopState::AclUpdate;

                    AclFieldMask owned_fields {};
                    auto has_field = [&](auto field){
                        using Field = decltype(field);
                        owned_fields.set(Field::index);
                        return updated_fields.has<Field>();
                    };

                    if (has_field(cfg::translation::language())) {
                        mod_factory.set_translator(translation_catalogs[this->ini.get<cfg::translation::language>()]);
                    }

                    if (has_field(cfg::context::session_id())) {
                        this->pid_file.rename(this->ini.get<cfg::context::session_id>());
                    }

                    if (has_field(cfg::context::module())
                     && ((ini.get<cfg::context::module>() != mod_factory.mod_name()
                       || ModuleName::waitinfo == mod_factory.mod_name()
                       || ini.get<cfg::context::try_alternate_target>())
                    )) {
                        next_module = ini.get<cfg::context::module>();
                        back_event = BACK_EVENT_NEXT;
                    }

                    if (has_field(cfg::context::keepalive())) {
                        keepalive.keep_alive();
                    }

                    if (has_field(cfg::context::end_date_cnx())) {
                        auto time_base = TimeBase::now();
                        event_manager.set_time_base(time_base);
                        const auto sys_date = time_base.real_time.time_since_epoch();
                        auto const elapsed = ini.get<cfg::context::end_date_cnx>() - sys_date;
                        auto const new_end_date = time_base.monotonic_time + elapsed;
                        end_session_warning.set_time(new_end_date);
                        mod_factory.set_time_close(new_end_date);
                    }

                    if (has_field(cfg::globals::inactivity_timeout())) {
                        set_inactivity_timeout(inactivity, ini);
                    }

                    if (has_field(cfg::audit::rt_display())) {
                        const Capture::RTDisplayResult rt_status =
                            front.set_rt_display(ini.get<cfg::audit::rt_display>(),
                                                 redis_params_from_ini(ini));

                        if (ini.get<cfg::client::enable_osd_4_eyes>()
                         && rt_status == Capture::RTDisplayResult::Enabled
                        ) {
                            zstring_view msg = mod_factory.tr(trkeys::enable_rt_display);
                            mod_factory.display_osd_message(msg.to_sv());
                        }
                    }

                    if (has_field(cfg::context::session_sharing_enable_control())) {
                        GuestCtx::ResultError result = {0xffff, "Sharing not available"};

                        if (front.is_up_and_running()
                         && mod_factory.is_connected()
                         && mod_factory.mod().is_up_and_running()
                        ) {
                            if (guest_ctx.is_started()) {
                                guest_ctx.stop();
                            }

                            result = guest_ctx.start(
                                app_path(AppPath::SessionTmpDir),
                                this->ini.get<cfg::context::session_id>(),
                                events,
                                front,
                                mod_factory.callback(),
                                secondary_session.get_secondary_session_log(),
                                this->ini.get<cfg::context::session_sharing_ttl>(),
                                rnd,
                                ini,
                                this->ini.get<cfg::context::session_sharing_enable_control>()
                            );
                        }

                        this->ini.send<cfg::context::session_sharing_userdata>();
                        this->ini.set_acl<cfg::context::session_sharing_invitation_error_code>(result.errnum);
                        if (result.errnum) {
                            this->ini.set_acl<cfg::context::session_sharing_invitation_error_message>(result.errmsg);
                        }
                        else {
                            this->ini.set_acl<cfg::context::session_sharing_invitation_addr>(guest_ctx.sck_path());
                            this->ini.set_acl<cfg::context::session_sharing_invitation_id>(
                                guest_ctx.sck_password().as<std::string_view>());
                            this->ini.set_acl<cfg::context::session_sharing_target_login>(
                                this->ini.get<cfg::globals::target_user>());
                            this->ini.set_acl<cfg::context::session_sharing_target_ip>(
                                SessionLogFile::get_target_ip(this->ini));
                        }
                    }

                    if (has_field(cfg::context::rejected())) {
                        LOG(LOG_ERR, "Connection is rejected by Authentifier! Reason: %s",
                            this->ini.get<cfg::context::rejected>().c_str());
                        err_msg_ctx.set_msg(this->ini.get<cfg::context::rejected>());
                        next_module = ModuleName::close;
                        back_event = BACK_EVENT_NEXT;
                    }
                    else if (has_field(cfg::context::disconnect_reason())) {
                        err_msg_ctx.set_msg(this->ini.get<cfg::context::disconnect_reason>());
                        this->ini.set_acl<cfg::context::disconnect_reason_ack>(true);
                        back_event = std::max(BACK_EVENT_NEXT, mod_factory.mod().get_mod_signal());
                    }
                    else if (!back_event) {
                        updated_fields.clear(owned_fields);
                        if (!updated_fields.is_empty()
                         && (next_module == ModuleName::UNKNOWN
                          || next_module == mod_factory.mod_name()
                        )) {
                            auto& mod = mod_factory.mod();
                            mod.acl_update(updated_fields);
                            back_event = std::max(back_event, mod.get_mod_signal());
                        }
                    }
                }


                loop_state = LoopState::EventLoop;

                if (!back_event) {
                    if (REDEMPTION_UNLIKELY(mod_has_data_to_write)) {
                        pmod_trans = mod_factory.mod_sck_transport();
                        if (pmod_trans && ioswitch.is_set_for_writing(pmod_trans->get_fd())) {
                            pmod_trans->send_waiting_data();
                        }
                    }

                    event_manager.execute_events(
                        [&ioswitch](int fd){
                            return fd >= 0 && ioswitch.is_set_for_reading(fd);
                        },
                        bool(this->verbose & SessionVerbose::Event));

                    back_event = mod_factory.mod().get_mod_signal();
                }
                else {
                    event_manager.execute_events(
                        [](int /*fd*/){ return false; },
                        bool(this->verbose & SessionVerbose::Event));
                }

                front.sync();


                loop_state = LoopState::BackEvent;

                if (REDEMPTION_UNLIKELY(back_event)) {
                    if (back_event == BACK_EVENT_STOP) {
                        LOG(LOG_INFO, "Module asked Front Disconnection");
                        break;
                    }

                    assert(back_event == BACK_EVENT_NEXT);

                    if (next_module == ModuleName::UNKNOWN) {
                        if (mod_factory.is_connected()) {
                            next_module = ModuleName::close;
                            this->ini.set_acl<cfg::context::module>(ModuleName::close);
                        }
                        else {
                            next_module = ModuleName::transitory;
                        }
                    }
                }


                loop_state = LoopState::NextMod;

                if (REDEMPTION_UNLIKELY(next_module != ModuleName::UNKNOWN)) {
                    if (is_close_module(next_module)) {
                        if (!ini.get<cfg::internal_mod::enable_close_box>()) {
                            LOG(LOG_INFO, "Close Box disabled: ending session");
                            break;
                        }
                    }

                    this->next_backend_module(
                        std::exchange(next_module, ModuleName::UNKNOWN),
                        secondary_session, err_msg_ctx, mod_factory, inactivity,
                        keepalive, front, guest_ctx, event_manager);
                }


                if (front.is_up_and_running()) {
                    if (front.has_user_activity) {
                        inactivity.activity();
                        front.has_user_activity = false;
                    }

                    end_session_warning.update_warning([&](std::chrono::minutes minutes){
                        if (ini.get<cfg::globals::enable_end_time_warning_osd>()
                         && mod_factory.mod().is_up_and_running()
                        ) {
                            loop_state = LoopState::UpdateOsd;
                            mod_factory.display_osd_message(Translator::FmtMsg<256>(
                                mod_factory.get_translator(),
                                trkeys::close_box_minute_timer,
                                static_cast<int>(minutes.count())
                            ).to_sv());
                        }
                    });
                }
            }
            catch (Error const& e)
            {
                bool run_session = false;

                switch (loop_state)
                {
                case LoopState::AclSend:
                    LOG(LOG_ERR, "ACL SERIALIZER: %s", e.errmsg());
                    err_msg_ctx.set_msg(trkeys::acl_fail);
                    auth_trans.disconnect();
                    break;

                case LoopState::AclReceive:
                    LOG(LOG_INFO, "acl_serial.incoming() Session lost");
                    err_msg_ctx.set_msg(trkeys::manager_close_cnx);
                    auth_trans.disconnect();
                    break;

                case LoopState::Select:
                    break;

                case LoopState::Front:
                    if (e.id == ERR_TRANSPORT_WRITE_FAILED
                     || e.id == ERR_TRANSPORT_NO_MORE_DATA
                    ) {
                        SocketTransport* socket_transport_ptr = mod_factory.mod_sck_transport();

                        if (socket_transport_ptr
                         && e.data == socket_transport_ptr->get_fd()
                         && ini.get<cfg::mod_rdp::auto_reconnection_on_losing_target_link>()
                         && mod_factory.mod().is_auto_reconnectable()
                         && !mod_factory.mod().server_error_encountered())
                        {
                            LOG(LOG_INFO, "Session::Session: target link exception. %s",
                                (ERR_TRANSPORT_WRITE_FAILED == e.id)
                                    ? "ERR_TRANSPORT_WRITE_FAILED"
                                    : "ERR_TRANSPORT_NO_MORE_DATA"
                            );

                            run_session = this->retry_rdp(
                                secondary_session, err_msg_ctx, mod_factory,
                                front, event_manager, PerformAutomaticReconnection::Yes);
                        }
                    }
                    else if (e.id == ERR_AUTOMATIC_RECONNECTION_REQUIRED) {
                        if (ini.get<cfg::mod_rdp::allow_session_reconnection_by_shortcut>()
                         && mod_factory.mod().is_auto_reconnectable())
                        {
                            LOG(LOG_INFO, "Session::Session: Automatic reconnection required. ERR_AUTOMATIC_RECONNECTION_REQUIRED");

                            run_session = this->retry_rdp(
                                secondary_session, err_msg_ctx, mod_factory,
                                front, event_manager, PerformAutomaticReconnection::Yes);
                        }
                    }
                    else {
                        // RemoteApp disconnection initiated by user
                        // ERR_DISCONNECT_BY_USER == e.id
                        if (// Can be caused by client disconnect.
                            e.id != ERR_X224_RECV_ID_IS_RD_TPDU
                         && e.id != ERR_MCS_APPID_IS_MCS_DPUM
                         && e.id != ERR_RDP_HANDSHAKE_TIMEOUT
                         // Can be caused by wabwatchdog.
                         && e.id != ERR_TRANSPORT_NO_MORE_DATA
                        ) {
                            LOG(LOG_ERR, "Proxy data processing raised error %u: %s",
                                e.id, e.errmsg(false));
                        }

                        front_trans.disconnect();
                    }

                    break;

                case LoopState::NextMod:
                    break;

                case LoopState::AclUpdate:
                case LoopState::EventLoop:
                case LoopState::BackEvent:
                case LoopState::UpdateOsd:
                    if (!front.is_up_and_running()) {
                        break;
                    }

                    switch (end_session_exception(e, ini, mod_factory))
                    {
                    case EndSessionResult::close_box: {
                        mod_factory.set_close_box_error_id(e.id);

                        if (ini.get<cfg::internal_mod::enable_close_box>()) {
                            if (!is_close_module(mod_factory.mod_name())) {
                                if (mod_factory.is_connected()) {
                                    this->ini.set_acl<cfg::context::module>(ModuleName::close);
                                }

                                this->next_backend_module(
                                    ModuleName::close, secondary_session, err_msg_ctx,
                                    mod_factory, inactivity, keepalive, front, guest_ctx,
                                    event_manager);
                                run_session = true;
                            }
                        }
                        else {
                            LOG(LOG_INFO, "Close Box disabled : ending session");
                        }
                        break;
                    }

                    case EndSessionResult::redirection: {
                        // SET new target in ini
                        auto& redir_info = mod_factory.get_redir_info();
                        const char * host = char_ptr_cast(redir_info.host);
                        const char * username = char_ptr_cast(redir_info.username);
                        const char * change_user = "";
                        if (redir_info.dont_store_username && username[0] != 0) {
                            LOG(LOG_INFO, "SrvRedir: Change target username to '%s'", username);
                            ini.set_acl<cfg::globals::target_user>(username);
                            change_user = username;
                        }
                        if (!redir_info.password_or_cookie.empty())
                        {
                            LOG(LOG_INFO, "SrvRedir: password or cookie");
                            std::vector<uint8_t>& redirection_password_or_cookie =
                                ini.get_mutable_ref<cfg::context::redirection_password_or_cookie>();

                            redirection_password_or_cookie = std::move(redir_info.password_or_cookie);
                        }
                        LOG(LOG_INFO, "SrvRedir: Change target host to '%s'", host);
                        ini.set_acl<cfg::context::target_host>(host);
                        auto message = str_concat(change_user, '@', host);
                        secondary_session.acl_report(AclReport::server_redirection(message));
                    }
                    [[fallthrough]];

                    // TODO: should we put some counter to avoid retrying indefinitely?
                    case EndSessionResult::retry:
                        run_session = this->retry_rdp(
                            secondary_session, err_msg_ctx, mod_factory,
                            front, event_manager, PerformAutomaticReconnection::No);
                        break;

                    // TODO: should we put some counter to avoid retrying indefinitely?
                    case EndSessionResult::reconnection:
                        run_session = this->retry_rdp(
                            secondary_session, err_msg_ctx, mod_factory,
                            front, event_manager, PerformAutomaticReconnection::Yes);
                        break;
                    }

                    break;
                }

                if (!run_session) {
                    break;
                }
            }
            catch (...)
            {
                switch (loop_state)
                {
                    case LoopState::AclSend:
                        LOG(LOG_ERR, "ACL SERIALIZER: unknown error");
                        err_msg_ctx.set_msg(trkeys::acl_fail);
                        auth_trans.disconnect();
                        break;

                    case LoopState::AclReceive:
                        LOG(LOG_INFO, "acl_serial.incoming() Session lost");
                        err_msg_ctx.set_msg(trkeys::manager_close_cnx);
                        auth_trans.disconnect();
                        break;

                    case LoopState::Select:
                        break;

                    case LoopState::Front:
                        LOG(LOG_ERR, "Proxy data processing raised an unknown error");
                        break;

                    case LoopState::NextMod:
                    case LoopState::AclUpdate:
                    case LoopState::EventLoop:
                    case LoopState::BackEvent:
                    case LoopState::UpdateOsd:
                        break;
                }

                break;
            }
        }

        try { acl_serial.send_acl_data(); }
        catch (...) {}

        try { front.sync(); }
        catch (...) {}

        guest_ctx.stop();

        const bool show_close_box = auth_trans.get_fd() == INVALID_SOCKET;
        if (!show_close_box || mod_factory.mod_name() != ModuleName::close) {
            mod_factory.disconnect();
        }
        front.must_be_stop_capture();

        return show_close_box && mod_factory.mod().get_mod_signal() == BACK_EVENT_NONE
             ? EndLoopState::ShowCloseBox
             : EndLoopState::ImmediateDisconnection;
    }

public:
    Session(
        SocketTransport&& front_trans, MonotonicTimePoint sck_start_time,
        Inifile& ini, PidFile& pid_file, Font const& font,
        TranslationCatalogsRef translation_catalogs, bool prevent_early_log
    )
    : ini(ini)
    , pid_file(pid_file)
    , verbose(safe_cast<SessionVerbose>(ini.get<cfg::debug::session>()))
    {
        CryptoContext cctx;
        URandom rnd;

        EventManager event_manager;
        event_manager.set_time_base(TimeBase::now());

        AclReporter acl_reporter{ini};
        SessionFront front(event_manager.get_events(), acl_reporter, front_trans, rnd, ini, cctx);

        ErrorMessageCtx err_msg_ctx;
        auto const log_translation = translation_catalogs[Language::en];
        auto log_disconnection = [&]{
            log_siem::disconnection(err_msg_ctx.visit_msg(
                [](TrKey const* k, zstring_view msg){
                    return k ? MsgTranslationCatalog::default_catalog().msgid(*k) : msg;
                }
            ));
        };

        int auth_sck = INVALID_SOCKET;

        LOG_IF(bool(this->verbose & SessionVerbose::Trace),
            LOG_INFO, "Session: Wait front.is_up_and_running()");

        try {
            null_mod no_mod;
            bool is_connected = this->internal_front_loop(
                front, front_trans, event_manager, no_mod,
                [&]{
                    return front.is_up_and_running();
                });

            if (is_connected) {
                if (unique_fd client_sck = addr_connect_blocking(
                    ini.get<cfg::globals::authfile>().c_str(),
                    ini.get<cfg::all_target_mod::connection_establishment_timeout>(),
                    prevent_early_log)
                ) {
                    auth_sck = client_sck.release();

                    ini.set_acl<cfg::globals::front_connection_time>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            MonotonicTimePoint::clock::now() - sck_start_time));

                    debug_data.save_and_reset_ini(ini, front.get_client_info().username);
                }
                else {
                    err_msg_ctx.set_msg(trkeys::err_sesman_unavailable);
                }
            }
        }
        catch (Error const& e) {
            // RemoteApp disconnection initiated by user
            // ERR_DISCONNECT_BY_USER == e.id
            if (// Can be caused by client disconnect.
                e.id != ERR_X224_RECV_ID_IS_RD_TPDU
             && e.id != ERR_MCS_APPID_IS_MCS_DPUM
             && e.id != ERR_RDP_HANDSHAKE_TIMEOUT
                // Can be caused by wabwatchdog.
             && e.id != ERR_TRANSPORT_NO_MORE_DATA
            ) {
                LOG(LOG_ERR, "Proxy data processing raised error %u : %s", e.id, e.errmsg(false));
            }
            return ;
        }
        catch (const std::exception & e) {
            LOG(LOG_ERR, "Proxy data processing raised error %s!", e.what());
            return ;
        }
        catch (...) {
            LOG(LOG_ERR, "Proxy data processing raised an unknown error");
            return ;
        }

        if (auth_sck == INVALID_SOCKET
         && (!front.is_up_and_running() || !ini.get<cfg::internal_mod::enable_close_box>())
        ) {
            // silent message for localhost or probe IPs for watchdog
            if (!prevent_early_log) {
                log_disconnection();
            }

            return ;
        }

        this->acl_auth_info(front.get_client_info());

        try {
            ModFactory mod_factory(
                event_manager.get_events(), event_manager.get_time_base(),
                front.get_client_info(), front, front, front.get_palette(),
                font, ini, front.keymap, rnd, cctx, err_msg_ctx,
                translation_catalogs[ini.get<cfg::translation::language>()],
                log_translation
            );

            GuestCtx guest_ctx;

            auto end_loop = EndLoopState::ShowCloseBox;

            if (auth_sck != INVALID_SOCKET) {
                end_loop = this->main_loop(
                    auth_sck, acl_reporter, event_manager, err_msg_ctx, cctx, rnd,
                    front_trans, front, guest_ctx,
                    mod_factory, translation_catalogs
                );
            }

            if (end_loop == EndLoopState::ShowCloseBox
             && front.is_up_and_running()
             && ini.get<cfg::internal_mod::enable_close_box>()
            ) {
                if (mod_factory.mod_name() != ModuleName::close) {
                    mod_factory.create_close_mod();
                }

                auto& mod = mod_factory.mod();
                this->internal_front_loop(
                    front, front_trans, event_manager, mod,
                    [&]{
                        return mod.get_mod_signal() != BACK_EVENT_NONE
                            || !front.is_up_and_running();
                    });
            }

            if (front.is_up_and_running()) {
                front.disconnect();
            }
        }
        catch (Error const& e) {
            if (e.id != ERR_TRANSPORT_WRITE_FAILED) {
                LOG(LOG_INFO, "Session Init exception %s", e.errmsg());
            }
        }
        catch (const std::exception & e) {
            LOG(LOG_ERR, "Session exception %s!", e.what());
        }
        catch (...) {
            LOG(LOG_ERR, "Session unexpected exception");
        }

        if (!ini.is_asked<cfg::globals::host>()) {
            LOG(LOG_INFO, "Client Session Disconnected");
        }
        log_disconnection();

        front.must_be_stop_capture();
    }

    Session(Session const &) = delete;
};

template<class SocketType, class... Args>
void session_start_sck(
    SocketTransport::Name name, unique_fd&& sck,
    MonotonicTimePoint sck_start_time, Inifile& ini,
    PidFile& pid_file, Font const& font,
    TranslationCatalogsRef translation_catalogs,
    bool prevent_early_log, Args&&... args)
{
    fcntl(sck.fd(), F_SETFL, fcntl(sck.fd(), F_GETFL) | O_NONBLOCK);

    auto const watchdog_verbosity = prevent_early_log
        ? SocketTransport::Verbose::watchdog
        : SocketTransport::Verbose();
    auto const sck_verbosity = safe_cast<SocketTransport::Verbose>(
        ini.get<cfg::debug::sck_front>());

    Session session(
        SocketType(
            name, std::move(sck), ""_av, 0,
            ini.get<cfg::all_target_mod::connection_establishment_timeout>(),
            ini.get<cfg::all_target_mod::tcp_user_timeout>(),
            ini.get<cfg::client::recv_timeout>(),
            static_cast<Args&&>(args)..., sck_verbosity | watchdog_verbosity
        ),
        sck_start_time, ini, pid_file, font, translation_catalogs, prevent_early_log
    );
}

} // anonymous namespace

void session_start_tls(unique_fd sck, MonotonicTimePoint sck_start_time, Inifile& ini, PidFile& pid_file, Font const& font, TranslationCatalogsRef translation_catalogs, bool prevent_early_log)
{
    session_start_sck<FinalSocketTransport>(
        "RDP Client"_sck_name, std::move(sck), sck_start_time, ini, pid_file, font,
        translation_catalogs, prevent_early_log);
}

void session_start_ws(unique_fd sck, MonotonicTimePoint sck_start_time, Inifile& ini, PidFile& pid_file, Font const& font, TranslationCatalogsRef translation_catalogs, bool prevent_early_log)
{
    session_start_sck<WsTransport>(
        "RDP Ws Client"_sck_name, std::move(sck), sck_start_time, ini, pid_file, font,
        translation_catalogs, prevent_early_log,
        WsTransport::UseTls::No, WsTransport::TlsOptions());
}

void session_start_wss(unique_fd sck, MonotonicTimePoint sck_start_time, Inifile& ini, PidFile& pid_file, Font const& font, TranslationCatalogsRef translation_catalogs, bool prevent_early_log)
{
    session_start_sck<WsTransport>(
        "RDP Wss Client"_sck_name, std::move(sck), sck_start_time, ini, pid_file, font,
        translation_catalogs, prevent_early_log,
        WsTransport::UseTls::Yes, WsTransport::TlsOptions{
            ini.get<cfg::globals::certificate_password>(),
            TlsConfig{
                .min_level = ini.get<cfg::client::tls_min_level>(),
                .max_level = ini.get<cfg::client::tls_max_level>(),
                .cipher_list = ini.get<cfg::client::ssl_cipher_list>(),
                .tls_1_3_ciphersuites = ini.get<cfg::client::tls_1_3_ciphersuites>(),
                .key_exchange_groups = ini.get<cfg::client::tls_key_exchange_groups>(),
                .signature_algorithms = ini.get<cfg::client::tls_signature_algorithms>(),
                .show_common_cipher_list = ini.get<cfg::client::show_common_cipher_list>(),
            },
        }
    );
}
