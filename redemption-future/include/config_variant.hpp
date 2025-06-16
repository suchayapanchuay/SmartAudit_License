/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Product name: redemption, a FLOSS RDP proxy
*   Copyright (C) Wallix 2013-2017
*   Author(s): Jonathan Poelen
*/

#pragma once

#define REDEMPTION_CONFIG_AUTHFILE "/tmp/redemption-sesman-sock" /*NOLINT*/
#define REDEMPTION_CONFIG_VALIDATOR_PATH "/tmp/redemption-validator-sock" /*NOLINT*/
#define REDEMPTION_CONFIG_ENABLE_WAB_INTEGRATION 0 /*NOLINT*/
#define REDEMPTION_CONFIG_SESSION_PROBE_ARGUMENTS "/K" /*NOLINT*/
#define REDEMPTION_CONFIG_THEME_LOGO "/usr/local/share/rdpproxy/wablogo.png" /*NOLINT*/

#define REDEMPTION_CONFIG_APPLICATION_DRIVER_EXE_OR_FILE "app_driver" /*NOLINT*/
#define REDEMPTION_CONFIG_APPLICATION_DRIVER_SCRIPT_ARGUMENT "${SCRIPT}" /*NOLINT*/
#define REDEMPTION_CONFIG_APPLICATION_DRIVER_CHROME_DT_SCRIPT "chrome_dt" /*NOLINT*/
#define REDEMPTION_CONFIG_APPLICATION_DRIVER_CHROME_UIA_SCRIPT "chrome_uia" /*NOLINT*/
#define REDEMPTION_CONFIG_APPLICATION_DRIVER_FIREFOX_UIA_SCRIPT "firefox_uia" /*NOLINT*/
#define REDEMPTION_CONFIG_APPLICATION_DRIVER_IE_SCRIPT "ie" /*NOLINT*/
