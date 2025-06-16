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
Copyright (C) Wallix 2010-2018
Author(s): Jonathan Poelen
*/

#pragma once

#include "mod/mod_api.hpp"
#include "mod/rdp/mod_rdp_variables.hpp"

#include <memory>

class EventContainer;
class ClientInfo;
class FrontAPI;
class LicenseApi;
class Transport;
class RedirectionInfo;
class Random;
class ModRDPParams;
class FileValidatorService;
class TlsConfig;
class ModRdpFactory;
class ChannelsAuthorizations;
class SessionLogApi;
class ErrorMessageCtx;

namespace gdi {
    class GraphicApi;
    class OsdApi;
}

std::unique_ptr<mod_api> new_mod_rdp(
    Transport& trans,
    gdi::GraphicApi& gd,
    gdi::OsdApi& osd,
    EventContainer& events,
    SessionLogApi& session_log,
    ErrorMessageCtx & err_msg_ctx,
    FrontAPI& front,
    const ClientInfo& info,
    RedirectionInfo& redir_info,
    Random& gen,
    const ChannelsAuthorizations& channels_authorizations,
    const ModRDPParams& mod_rdp_params,
    const TlsConfig& tls_config,
    LicenseApi& license_store,
    ModRdpVariables vars,
    FileValidatorService * file_validator_service,
    ModRdpFactory& mod_rdp_factory
);
