#include "config_variant.hpp"

R"gen_config_ini(## Config file for RDP proxy.


[globals]

# Port of RDP Proxy service.
# 
# Changing the port number will restart the service and disconnect active sessions. It will also block WALLIX Access Manager connections unless its RDP port is configured.
# Choose a port that is not already in use. Otherwise, the service will not run.
# (min = 0)
#_advanced
#port = 3389

# Socket path or socket address of passthrough / ACL.
#authfile = )gen_config_ini" << (REDEMPTION_CONFIG_AUTHFILE) << R"gen_config_ini(

# Timeout during RDP connection initialization.
# Increase this value if the connection between workstations and Bastion is slow.
# (in seconds)
#handshake_timeout = 10

# No automatic disconnection due to inactivity, timer is set on primary authentication.
# If the value is between 1 and 30, then 30 is used.
# If the value is set to 0, then inactivity timeout value is unlimited.
# (in seconds)
#base_inactivity_timeout = 900

# No automatic disconnection due to inactivity, timer is set on target session.
# If the value is between 1 and 30, then 30 is used.
# If the value is set to 0, then the value set in [globals]base_inactivity_timeout is used.
# (in seconds)
# (acl config: proxy ⇐ globals:inactivity_timeout)
#inactivity_timeout = 0

# Internal keepalive between ACL and RDP proxy.
# (in seconds)
#keepalive_grace_delay = 30

# Specifies how long the RDP proxy login screen should be displayed before the client window closes (use 0 to deactivate).
# (in seconds)
#_advanced
#authentication_timeout = 120

# Specify bind address.
#listen_address = 0.0.0.0

# Transparent mode allows network traffic interception for a target, even when users directly specify a target's address instead of a proxy address.
# (type: boolean (0/no/false or 1/yes/true))
#enable_transparent_mode = 0

# Proxy certificate password.
# (maxlen = 254)
#certificate_password = inquisition

# Support of Bitmap Update.
# (type: boolean (0/no/false or 1/yes/true))
#enable_bitmap_update = 1

# Displays a reminder box at the top of the session when a session has a time limit (timeframe or approval).
# Reminders appear at 30 minutes, 10 minutes, 5 minutes, and 1 minute before the session ends.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Enable end time warning OSD
#enable_end_time_warning_osd = 1

# Allows showing the target device name with F12 during the session.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Enable OSD display remote target
# (acl config: proxy ⇐ enable_osd_display_remote_target)
#enable_osd_display_remote_target = 1

# Displays the target username in the session when F12 is pressed.
# This option needs [globals]enable_osd_display_remote_target.
# (type: boolean (0/no/false or 1/yes/true))
#show_target_user_in_f12_message = 0

# (type: boolean (0/no/false or 1/yes/true))
#enable_wab_integration = )gen_config_ini" << (REDEMPTION_CONFIG_ENABLE_WAB_INTEGRATION) << R"gen_config_ini(

# Enable support for 96x96 size pointers.
# ⚠ If this option is disabled and the application does not support smaller pointers, the pointer may remain stuck on the last active pointer. For example, the resize window pointer would remain visible rather than change to a 'normal' pointer.
# (type: boolean (0/no/false or 1/yes/true))
#large_pointer_support = 1

# (in milliseconds | min = 100, max = 10000)
#mod_recv_timeout = 1000

# (type: boolean (0/no/false or 1/yes/true))
#experimental_enable_serializer_data_block_size_limit = 0

# Prevent Remote Desktop session timeouts due to idle TCP sessions by periodically sending keepalive packets to the client.
# Set to 0 to disable this feature.
# (in milliseconds)
#_display_name=RDP keepalive connection interval
#rdp_keepalive_connection_interval = 0

# ⚠ Manually restart service redemption to take changes into account.
# 
# Enable primary connection on IPv6.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Enable IPv6
#enable_ipv6 = 1

# 0 for disabled.
# (in megabytes)
#minimal_memory_available_before_connection_silently_closed = 100

[client]

# If true, ignore the password provided by the RDP client. The user needs to manually log in.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#ignore_logon_password = 0

# Sends the client screen count to the target server. Not supported for VNC targets.
# Uncheck to disable multiple monitors.
# (type: boolean (0/no/false or 1/yes/true))
#allow_using_multiple_monitors = 1

# Sends Scale & Layout configuration to the target server.
# On Windows 11, this corresponds to the options "Scale", "Display Resolution" and "Display Orientation" of Settings > System > Display.
# ⚠ Title bar detection via OCR will no longer work.
# (type: boolean (0/no/false or 1/yes/true))
#allow_scale_factor = 0

# Allows the client to use unicode characters.
# This is useful for displaying characters that are not available on the keyboard layout used, such as some special characters or emojis.
# (type: boolean (0/no/false or 1/yes/true))
#unicode_keyboard_event_support = 1

# Legacy encryption when External Security Protocol (TLS, CredSSP, etc) is disabled.
# values: none, low, medium, high
#encryption_level = high

# Fallback to RDP Legacy Encryption if the client does not support TLS.
# ⚠ Enabling this option is a security risk.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=TLS fallback legacy
#tls_fallback_legacy = 0

# Enable TLS between client and proxy.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=TLS support
#tls_support = 1

# Minimal incoming TLS level: 0=TLSv1, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3
# ⚠ Lower this value only for compatibility reasons.
# (min = 0)
#_display_name=TLS min level
#tls_min_level = 2

# Maximal incoming TLS level: 0=no restriction, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3
# ⚠ Change this value only for compatibility reasons.
# (min = 0)
#_display_name=TLS max level
#tls_max_level = 0

# [Not configured]: Compatible with more RDP clients (less secure)
# HIGH:!ADH:!3DES: Compatible only with MS Windows 7 client or more recent (moderately secure)
# HIGH:!ADH:!3DES:!SHA: Compatible only with MS Server Windows 2008 R2 client or more recent (more secure)
# The format used is described on this page: https://www.openssl.org/docs/man3.1/man1/openssl-ciphers.html#CIPHER-LIST-FORMAT
#_display_name=SSL cipher list
#ssl_cipher_list = HIGH:!ADH:!3DES:!SHA

# Configure the available TLSv1.3 ciphersuites.
# Empty to apply system-wide configuration.
# The format used is described in the third paragraph of this page: https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_ciphersuites.html#DESCRIPTION
#_display_name=TLS 1.3 cipher suites
#tls_1_3_ciphersuites = 

# Configure the supported key exchange groups.
# Empty to apply system-wide configuration.
# The format used is described in this page: https://www.openssl.org/docs/man3.2/man3/SSL_CONF_cmd.html#groups-groups
#_display_name=TLS key exchange groups
#tls_key_exchange_groups = 

# Configure the supported server signature algorithms.
# Empty to apply system-wide configuration.
# The format should be a colon separated list of signature algorithms in order of decreasing preference of the form algorithm+hash or signature_scheme.
# algorithm is one of RSA, RSA-PSS or ECDSA.
# hash is one of SHA224, SHA256, SHA384 or SHA512.
# signature_scheme is one of the signature schemes defined in TLSv1.3 (rfc8446#section-4.2.3), specified using the IETF name, e.g., ecdsa_secp384r1_sha384 or rsa_pss_rsae_sha256.
# This list needs at least one signature algorithm compatible with the RDP Proxy certificate.
#_display_name=TLS signature algorithms
#tls_signature_algorithms = RSA+SHA256:RSA+SHA384:RSA+SHA512:RSA-PSS+SHA256:RSA-PSS+SHA384:RSA-PSS+SHA512:ECDSA+SHA256:ECDSA+SHA384:ECDSA+SHA512

# Show the common cipher list supported by client and target server in the logs.
# ⚠ Only for debugging purposes.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#show_common_cipher_list = 0

# Needed for primary NTLM or Kerberos connections over NLA.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable NLA
#enable_nla = 0

# Specifies the highest RDP compression support available on the client connection session.
#   0: The RDP bulk compression is disabled
#   1: RDP 4.0 bulk compression
#   2: RDP 5.0 bulk compression
#   3: RDP 6.0 bulk compression
#   4: RDP 6.1 bulk compression
#_advanced
#_display_name=RDP compression
#rdp_compression = 4

# Specifies the maximum color depth for the client connection session:
#   8: 8-bit
#   15: 15-bit 555 RGB mask
#   16: 16-bit 565 RGB mask
#   24: 24-bit RGB mask
#   32: 32-bit RGB mask + alpha
#_advanced
#max_color_depth = 24

# Persistent Disk Bitmap Cache on the primary connection side. If the RDP client supports it, this setting increases the size of image caches.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#persistent_disk_bitmap_cache = 1

# Support of Cache Waiting List: Experimental cache strategy (this value is ignored if [client]persistent_disk_bitmap_cache is disabled).
# (type: boolean (0/no/false or 1/yes/true))
#cache_waiting_list = 0

# If enabled, the content of Persistent Bitmap Caches are stored on the disk to reuse later (this value is ignored if [client]persistent_disk_bitmap_cache is disabled).
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#persist_bitmap_cache_on_disk = 0

# Enable Bitmap Compression when supported by the RDP client.
# Disabling this option increases the network bandwith usage.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#bitmap_compression = 1

# Enables support of Client Fast-Path Input Event PDUs.
# (type: boolean (0/no/false or 1/yes/true))
#fast_path = 1

# Allows the client to request the target server to stop graphical updates. For example, when the RDP client window is minimized to reduce bandwidth.
# ⚠ If changes occur on the target, they will not be visible in the recordings either.
# (type: boolean (0/no/false or 1/yes/true))
#enable_suppress_output = 1

# Same effect as [client]transform_glyph_to_bitmap, but only for RDP clients on an iOS platform.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Bogus iOS glyph support level
#bogus_ios_glyph_support_level = 1

# This option converts glyphs to bitmaps to resolve issues with certain RDP clients.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#transform_glyph_to_bitmap = 0

# (in milliseconds | min = 100, max = 10000)
#recv_timeout = 1000

# Informs users with a message when their session is audited.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable OSD 4 eyes
#enable_osd_4_eyes = 1

# Enable RemoteFX on the client connection.
# Needs - [client]max_color_depth set to 32 (32-bit RGB mask + alpha)
#       - [mod_rdp]enable_remotefx set to on
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Enable RemoteFX
#enable_remotefx = 1

# This option should only be used if the target server or client is showing graphical issues.
# In general, disabling RDP orders has a negative impact on performance.
# 
# Drawing orders that can be disabled:
#    0: DstBlt
#    1: PatBlt
#    2: ScrBlt
#    3: MemBlt
#    4: Mem3Blt
#    9: LineTo
#   15: MultiDstBlt
#   16: MultiPatBlt
#   17: MultiScrBlt
#   18: MultiOpaqueRect
#   22: Polyline
#   25: EllipseSC
#   27: GlyphIndex
# (values are comma-separated)
#_advanced
#disabled_orders = 25

# This option fixes a bug in the Remote Desktop client on Windows 11/Windows Server 2025 starting with version 24H2.
# Occasionally, some screen areas may not refresh due to a bug where only the first image in a BitmapUpdate message with multiple images is displayed correctly.
# Enabling this option prevents this issue, but will slightly increase the data sent to the client.
# The option is automatically disabled if the connection is from an Access Manager.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#workaround_incomplete_images = 0

[all_target_mod]

# The maximum wait time for the proxy to connect to a target.
# (in milliseconds | min = 1000, max = 10000)
#_advanced
#connection_establishment_timeout = 3000

# This parameter allows you to specify max timeout before a TCP connection is aborted. If the option value is specified as 0, TCP will use the system default.
# (in milliseconds | min = 0, max = 3600000)
#_advanced
#_display_name=TCP user timeout
# (acl config: proxy ⇐ all_target_mod:tcp_user_timeout)
#tcp_user_timeout = 0

[remote_program]

# Allows resizing of a desktop session opened in a RemoteApp window.
# This happens when an RDP client opened in RemoteApp accesses a desktop target.
# (type: boolean (0/no/false or 1/yes/true))
#allow_resize_hosted_desktop = 1

[mod_rdp]

# It specifies a list of (comma-separated) RDP server desktop features to enable or disable in the session (with the goal of optimizing bandwidth usage).
# 
# If a feature is preceded by a "-" sign, it is disabled; if it is preceded by a "+" sign or no sign, it is enabled. Unconfigured features can be controlled by the RDP client.
# 
# Available features:
#   - wallpaper
#   - full_window_drag
#   - menu_animations
#   - theme
#   - mouse_cursor_shadows
#   - cursor_blinking
#   - font_smoothing
#   - desktop_composition
#_advanced
#force_performance_flags = -mouse_cursor_shadows,-theme,-menu_animations

# If enabled, the font smoothing desktop feature is automatically disabled in recorded session.
# This allows OCR (when session probe is disabled) to better detect window titles.
# If disabled, it allows font smoothing in recorded sessions. However, OCR will not work when session recording is disabled. In this case, window titles are not detected.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#auto_adjust_performance_flags = 1

# Specifies the highest RDP compression support available on the target server connection.
#   0: The RDP bulk compression is disabled
#   1: RDP 4.0 bulk compression
#   2: RDP 5.0 bulk compression
#   3: RDP 6.0 bulk compression
#   4: RDP 6.1 bulk compression
#_advanced
#_display_name=RDP compression
#rdp_compression = 4

# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#disconnect_on_logon_user_change = 0

# The maximum wait time for the proxy to log on to an RDP session.
# Value 0 is equivalent to 15 seconds.
# (in seconds)
#_advanced
#open_session_timeout = 0

# This option should only be used if the target server or client is showing graphical issues.
# In general, disabling RDP orders has a negative impact on performance.
# 
# Drawing orders that can be disabled:
#    0: DstBlt
#    1: PatBlt
#    2: ScrBlt
#    3: MemBlt
#    4: Mem3Blt
#    9: LineTo
#   15: MultiDstBlt
#   16: MultiPatBlt
#   17: MultiScrBlt
#   18: MultiOpaqueRect
#   22: Polyline
#   25: EllipseSC
#   27: GlyphIndex
# (values are comma-separated)
#_advanced
# (acl config: proxy ⇐ mod_rdp:disabled_orders)
#disabled_orders = 27

# Enable NLA authentication in secondary targets.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable NLA
# (acl config: proxy ⇐ mod_rdp:enable_nla)
#enable_nla = 1

# When [mod_rdp]enable_nla is selected:
# Enabling this option instructs the Bastion to use Kerberos as its initial method.
# Disabling this option instructs the Bastion to only use NTLM.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable Kerberos
# (acl config: proxy ⇐ mod_rdp:enable_kerberos)
#enable_kerberos = 1

# When both [mod_rdp]enable_nla and [mod_rdp]enable_kerberos are selected:
# Enabling this option instructs the Bastion to use Kerberos first and, if necessary, NTLM as a backup.
# Disabling this option instructs the Bastion to only use Kerberos.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Allow NLA NTLM fallback
# (acl config: proxy ⇐ mod_rdp:allow_nla_ntlm_fallback)
#allow_nla_ntlm_fallback = 0

# Allow TLS only.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Allow TLS only fallback
# (acl config: proxy ⇐ mod_rdp:allow_tls_only_fallback)
#allow_tls_only_fallback = 0

# Allow Standard RDP Security (Legacy).
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Allow RDP legacy fallback
# (acl config: proxy ⇐ mod_rdp:allow_rdp_legacy_fallback)
#allow_rdp_legacy_fallback = 0

# Minimal incoming TLS level 0=TLSv1, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3
# (min = 0)
#_display_name=TLS min level
# (acl config: proxy ⇐ mod_rdp:tls_min_level)
#tls_min_level = 2

# Maximal incoming TLS level 0=no restriction, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3
# (min = 0)
#_display_name=TLS max level
# (acl config: proxy ⇐ mod_rdp:tls_max_level)
#tls_max_level = 0

# TLSv1.2 and below additional ciphers supported.
# Empty to apply system-wide configuration (SSL security level 2), ALL for support of all ciphers to ensure highest compatibility with target servers.
# The format used is described on this page: https://www.openssl.org/docs/man3.1/man1/openssl-ciphers.html#CIPHER-LIST-FORMAT
# (acl config: proxy ⇐ mod_rdp:cipher_string)
#cipher_string = ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES128-GCM-SHA256

# Allow TLS legacy insecure renegotiation to unpatched target servers.
# For Windows Server 2008, requires also to set ALL@SECLEVEL=0 in [mod_rdp]cipher_string.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=TLS enable legacy server
# (acl config: proxy ⇐ mod_rdp:tls_enable_legacy_server)
#tls_enable_legacy_server = 0

# Configure the available TLSv1.3 ciphersuites.
# Empty to apply system-wide configuration.
# The format used is described in the third paragraph of this page: https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_ciphersuites.html#DESCRIPTION
#_display_name=TLS 1.3 cipher suites
# (acl config: proxy ⇐ mod_rdp:tls_1_3_ciphersuites)
#tls_1_3_ciphersuites = 

# Configure the supported key exchange groups.
# Empty to apply system-wide configuration.
# The format used is described in this page: https://www.openssl.org/docs/man3.2/man3/SSL_CONF_cmd.html#groups-groups
#_display_name=TLS key exchange groups
# (acl config: proxy ⇐ mod_rdp:tls_key_exchange_groups)
#tls_key_exchange_groups = P-256:P-384:P-521:ffdhe3072:ffdhe4096:ffdhe6144:ffdhe8192

# Show in the logs the common cipher list supported by client and target server
# ⚠ Only for debugging purposes.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ mod_rdp:show_common_cipher_list)
#show_common_cipher_list = 0

# Persistent Disk Bitmap Cache on the secondary connection side. If supported by the RDP target server, the size of image caches will be increased.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#persistent_disk_bitmap_cache = 1

# Support of Cache Waiting List (this value is ignored if the [mod_rdp]persistent_disk_bitmap_cache is disabled).
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#cache_waiting_list = 1

# If enabled, the contents of Persistent Bitmap Caches are stored on disk for reusing them later (this value is ignored if [mod_rdp]persistent_disk_bitmap_cache is disabled).
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#persist_bitmap_cache_on_disk = 0

# List of (comma-separated) enabled (static) virtual channel. If character '*' is used as a name then enables everything.
# Explicit names have higher priority than '*' in [mod_rdp]denied_channels.
# (values are comma-separated)
# (acl config: proxy ⇐ mod_rdp:allowed_channels)
#allowed_channels = *

# List of (comma-separated) disabled (static) virtual channel. If character '*' is used as a name then disables everything.
# Explicit names have higher priority than '*' in [mod_rdp]allowed_channels.
# (values are comma-separated)
# (acl config: proxy ⇐ mod_rdp:denied_channels)
#denied_channels = 

# List of (comma-separated) enabled dynamic virtual channel. If character '*' is used as a name then enables everything.
# Explicit names have higher priority than '*' in [mod_rdp]denied_dynamic_channels.
#_advanced
# (acl config: proxy ⇐ mod_rdp:allowed_dynamic_channels)
#allowed_dynamic_channels = *

# List of (comma-separated) disabled dynamic virtual channel. If character '*' is used as a name then disables everything.
# Explicit names have higher priority than '*' in [mod_rdp]allowed_dynamic_channels.
#_advanced
# (acl config: proxy ⇐ mod_rdp:denied_dynamic_channels)
#denied_dynamic_channels = 

# If this option is unchecked, keyboard/mouse inputs are transmitted over the dynamic virtual channel.
# This will cause the session inactivity detection and keyboard input log to malfunction in Windows 11 and Windows Server 2025.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ mod_rdp:disable_coreinput_dynamic_channel)
#disable_coreinput_dynamic_channel = 1

# Force the screen resolution of a RDP target.
# This option is ignored when the value is 0x0 or empty.
# (in {width}x{height} format (e.g. 800x600))
# (acl config: proxy ⇐ mod_rdp:force_screen_resolution)
#force_screen_resolution = 

# Enables support of Client/Server Fast-Path Input/Update PDUs.
# Fast-Path is required for Windows Server 2012 (or more recent).
# (type: boolean (0/no/false or 1/yes/true))
#fast_path = 1

# The secondary target connection can be redirected to a specific session on another RDP server.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable Server Redirection Support
# (acl config: proxy ⇐ mod_rdp:server_redirection_support)
#server_redirection_support = 0

# Load balancing information.
# For example 'tsv://MS Terminal Services Plugin.1.Sessions' where 'Sessions' is the name of the targeted RD Collection which works fine.
# (acl config: proxy ⇐ mod_rdp:load_balance_info)
#load_balance_info = 

# Client Address to send to target (in InfoPacket).
#   0: Send 0.0.0.0
#   1: Send proxy client address or target connexion.
#   2: Send user client address of front connexion.
#_advanced
#client_address_sent = 0

# Shared directory between proxy and secondary target.
# Requires RDPDR support.
# (values are comma-separated)
# (acl config: proxy ⇐ mod_rdp:proxy_managed_drives)
#proxy_managed_drives = 

# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ mod_rdp:ignore_auth_channel)
#ignore_auth_channel = 0

# Authentication channel used by Auto IT scripts. May be '*' to use default name. Keep empty to disable virtual channel.
# (maxlen = 7)
#auth_channel = *

# Authentication channel used by other scripts. No default name. Keep empty to disable virtual channel.
# (maxlen = 7)
#checkout_channel = 

# (acl config: proxy ⇐ alternate_shell)
#alternate_shell = 

# (acl config: proxy ⇐ shell_arguments)
#shell_arguments = 

# (acl config: proxy ⇐ shell_working_directory)
#shell_working_directory = 

# As far as possible, use client-provided initial program (Alternate Shell).
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ mod_rdp:use_client_provided_alternate_shell)
#use_client_provided_alternate_shell = 0

# As far as possible, use client-provided remote program (RemoteApp).
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Use client provided RemoteApp
# (acl config: proxy ⇐ mod_rdp:use_client_provided_remoteapp)
#use_client_provided_remoteapp = 0

# As far as possible, use native RemoteApp capability.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Use native RemoteApp capability
# (acl config: proxy ⇐ mod_rdp:use_native_remoteapp_capability)
#use_native_remoteapp_capability = 1

# (maxlen = 256)
#application_driver_exe_or_file = )gen_config_ini" << (REDEMPTION_CONFIG_APPLICATION_DRIVER_EXE_OR_FILE) << R"gen_config_ini(

# (maxlen = 256)
#application_driver_script_argument = )gen_config_ini" << (REDEMPTION_CONFIG_APPLICATION_DRIVER_SCRIPT_ARGUMENT) << R"gen_config_ini(

# (maxlen = 256)
#application_driver_chrome_dt_script = )gen_config_ini" << (REDEMPTION_CONFIG_APPLICATION_DRIVER_CHROME_DT_SCRIPT) << R"gen_config_ini(

# (maxlen = 256)
#application_driver_chrome_uia_script = )gen_config_ini" << (REDEMPTION_CONFIG_APPLICATION_DRIVER_CHROME_UIA_SCRIPT) << R"gen_config_ini(

# (maxlen = 256)
#application_driver_firefox_uia_script = )gen_config_ini" << (REDEMPTION_CONFIG_APPLICATION_DRIVER_FIREFOX_UIA_SCRIPT) << R"gen_config_ini(

# (maxlen = 256)
#application_driver_ie_script = )gen_config_ini" << (REDEMPTION_CONFIG_APPLICATION_DRIVER_IE_SCRIPT) << R"gen_config_ini(

# Do not transmit the client machine name to the RDP server.
# If Per-Device licensing mode is configured on the RD host, this Bastion will consume a CAL for all of these connections to the RD host.
# (type: boolean (0/no/false or 1/yes/true))
#hide_client_name = 0

# Stores CALs issued by the terminal servers.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#use_license_store = 1

# Workaround option to support partial clipboard initialization performed by some versions of FreeRDP.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Bogus FreeRDP clipboard
#bogus_freerdp_clipboard = 0

# Workaround option to disable shared disk for RDP client on iOS platform only.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Bogus iOS RDPDR virtual channel
#bogus_ios_rdpdr_virtual_channel = 1

# Workaround option to fix some drawing issues with Windows Server 2012.
# Can be disabled when none of the targets are Windows Server 2012.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#bogus_refresh_rect = 1

# Adds RDPDR channel metadata to session logs. Disabling this option makes shared disks more responsive, but metadata will no longer be collected.if at least one authorization of RDPDR is missing (Printer, ComPort, SmartCard, Drive), then this option is considered enabled.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Enable RDPDR data analysis
# (acl config: proxy ⇐ mod_rdp:enable_rdpdr_data_analysis)
#enable_rdpdr_data_analysis = 1

# Delay before automatically bypassing Windows's Legal Notice screen in RemoteApp mode.
# Set to 0 to disable this feature.
# (in milliseconds)
#_advanced
#_display_name=RemoteApp bypass legal notice delay
#remoteapp_bypass_legal_notice_delay = 0

# Time limit to automatically bypass Windows's Legal Notice screen in RemoteApp mode.
# Set to 0 to disable this feature.
# (in milliseconds)
#_advanced
#_display_name=RemoteApp bypass legal notice timeout
#remoteapp_bypass_legal_notice_timeout = 20000

# Some events such as 'Preferred DropEffect' have no particular meaning. This option allows you to exclude these types of events from the logs.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#log_only_relevant_clipboard_activities = 1

# Force splitting target domain and username with '@' separator.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#split_domain = 0

# Actives conversion of RemoteApp target session to desktop session.
# Otherwise, Alternate Shell will be used.
# Some Windows Shell features may be unavailable in one or both cases, and applications using them may behave differently.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable translated RemoteApp with AM
# (acl config: proxy ⇐ mod_rdp:wabam_uses_translated_remoteapp)
#wabam_uses_translated_remoteapp = 0

# Enables Session Shadowing Support.
# Session probe must be enabled on target connection policy.
# Target server must support "Remote Desktop Shadowing" feature.
# When enabled, users can share their RDP sessions with auditors who request it.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#session_shadowing_support = 1

# Enables support of the RemoteFX codec on target connection.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable RemoteFX
# (acl config: proxy ⇐ mod_rdp:enable_remotefx)
#enable_remotefx = 0

# Connect to the target server in Restricted Admin mode.
# This mode must be supported by the target server (available from Windows Server 2012 R2), otherwise, connection will fail.
# NLA must be enabled.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ mod_rdp:enable_restricted_admin_mode)
#enable_restricted_admin_mode = 0

# NLA will be disabled.
# Target must be set for interactive login, otherwise the  connection may not be guaranteed.
# Smartcard device must be available on client desktop.
# Smartcard redirection (Proxy option RDP_SMARTCARD) must be enabled on service.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ mod_rdp:force_smartcard_authentication)
#force_smartcard_authentication = 0

# Enable target connection on IPv6
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable IPv6
# (acl config: proxy ⇐ mod_rdp:enable_ipv6)
#enable_ipv6 = 1

# Console mode management for targets on Windows Server 2003 (requested with /console or /admin mstsc option).
#   allow: Forward Console mode request from client to the target.
#   force: Force Console mode on target regardless of client request.
#   forbid: Block Console mode request from client.
#_display_name=Console mode
# (acl config: proxy ⇐ mod_rdp:mode_console)
#mode_console = allow

# Allows the proxy to automatically reconnect to secondary target when a network error occurs.
# The target server must support reconnection cookie.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ mod_rdp:auto_reconnection_on_losing_target_link)
#auto_reconnection_on_losing_target_link = 0

# ⚠ The use of this feature is not recommended!
# 
# If the feature is enabled, the end user can trigger a session disconnection/reconnection with the shortcut Ctrl+F12.
# This feature should not be used together with the [session_probe]end_disconnected_session.
# The keyboard shortcut is fixed and cannot be changed.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ mod_rdp:allow_session_reconnection_by_shortcut)
#allow_session_reconnection_by_shortcut = 0

# The delay between a session disconnection and the automatic reconnection that follows.
# (in milliseconds | min = 0, max = 15000)
#_advanced
# (acl config: proxy ⇐ mod_rdp:session_reconnection_delay)
#session_reconnection_delay = 0

# Forward the build number advertised by the client to the target server. If forwarding is disabled a default (static) build number will be sent to the target server.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ mod_rdp:forward_client_build_number)
#forward_client_build_number = 1

# To resolve the session freeze issue with Windows 7/Windows Server 2008 target.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ mod_rdp:bogus_monitor_layout_treatment)
#bogus_monitor_layout_treatment = 0

# Delay before showing disconnect message after the last RemoteApp window is closed.
# (in milliseconds | min = 3000, max = 120000)
#_advanced
# (acl config: proxy ⇐ mod_rdp:remote_programs_disconnect_message_delay)
#remote_programs_disconnect_message_delay = 3000

# This option only has an effect in RemoteApp sessions (RDS meaning).
# If enabled, the RDP Proxy relies on the Session Probe to launch the remote programs.
# Otherwise, remote programs will be launched according to Remote Programs Virtual Channel Extension of Remote Desktop Protocol. This latter is the native method.
# The difference is that Session Probe does not start a new application when its host session is resumed. Conversely, launching applications according to Remote Programs Virtual Channel Extension of Remote Desktop Protocol is not affected by this behavior. However, launching applications via the native method requires them to be published in Remote Desktop Services, which is unnecessary if launched by the Session Probe.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Use Session Probe to launch remote program
# (acl config: proxy ⇐ mod_rdp:use_session_probe_to_launch_remote_program)
#use_session_probe_to_launch_remote_program = 1

# ⚠ The use of this feature is not recommended!
# 
# Replace an empty mouse pointer with normal pointer.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ mod_rdp:replace_null_pointer_by_default_pointer)
#replace_null_pointer_by_default_pointer = 0

# Enable Windows XP clipboard support.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ mod_rdp:windows_xp_clipboard_support)
#windows_xp_clipboard_support = 0

# Block user input in the session during the execution of Application Driver. Do not activate this option if the application session does not use Application Driver.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Block user input until Application Driver completes
# (acl config: proxy ⇐ mod_rdp:block_user_input_until_appdriver_completes)
#block_user_input_until_appdriver_completes = 0

[protocol]

#   0: Windows
#   1: Bastion, xrdp or others
#_advanced
# (acl config: proxy ⇐ protocol:save_session_info_pdu)
#save_session_info_pdu = 1

[session_probe]

# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable Session Probe
# (acl config: proxy ⇐ session_probe:enable_session_probe)
#enable_session_probe = 0

# (maxlen = 511)
#exe_or_file = ||CMD

# (maxlen = 511)
#arguments = )gen_config_ini" << (REDEMPTION_CONFIG_SESSION_PROBE_ARGUMENTS) << R"gen_config_ini(

# This parameter only has an effect in Desktop sessions.
# It allows you to choose between Smart launcher and Legacy launcher to launch the Session Probe.
# The Smart launcher and the Legacy launcher do not have the same technical prerequisites. Detailed information can be found in the Administration guide.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ session_probe:use_smart_launcher)
#use_smart_launcher = 1

# This parameter enables or disables the Session Probe’s launch mask.
# The Launch mask hides the Session Probe launch steps from the end-users.
# Disabling the mask makes it easier to diagnose Session Probe launch issues. It is recommended to enable the mask for normal operation.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ session_probe:enable_launch_mask)
#enable_launch_mask = 1

# It is recommended to use option 1 (disconnect user).
#   0: ignore and continue: The metadata collected is not essential for us. Instead, we prefer to minimize the impact on the user experience. The Session Probe launch will be in best-effort mode. The prevailing duration is defined by the 'Launch fallback timeout' instead of the 'Launch timeout'.
#   1: disconnect user: This is the recommended setting. If the target meets all the technical prerequisites, there is no reason for the Session Probe not to launch. All that remains is to adapt the value of 'Launch timeout' to the performance of the target.
#   2: retry without session probe: We wish to be able to recover the behavior of Bastion 5 when the Session Probe does not launch. The prevailing duration is defined by the 'Launch fallback timeout' instead of the 'Launch timeout'.
# (acl config: proxy ⇐ session_probe:on_launch_failure)
#on_launch_failure = 1

# This parameter is used if [session_probe]on_launch_failure is 1 (disconnect user).
# 0 to disable timeout.
# (in milliseconds | min = 0, max = 300000)
#_advanced
# (acl config: proxy ⇐ session_probe:launch_timeout)
#launch_timeout = 40000

# This parameter is used if [session_probe]on_launch_failure is 0 (ignore failure and continue) or 2 (retry without Session Probe).
# 0 to disable timeout.
# (in milliseconds | min = 0, max = 300000)
#_advanced
# (acl config: proxy ⇐ session_probe:launch_fallback_timeout)
#launch_fallback_timeout = 40000

# If enabled, the [session_probe]launch_timeout countdown timer will be started only after user logged in Windows. Otherwise, the countdown timer will be started immediately after RDP protocol connexion.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ session_probe:start_launch_timeout_timer_only_after_logon)
#start_launch_timeout_timer_only_after_logon = 1

# The amount of time that RDP Proxy waits for a reply from the Session Probe to the KeepAlive message before adopting the behavior defined by [session_probe]on_keepalive_timeout.
# If our local network is subject to congestion, or if the Windows lacks responsiveness, it is possible to increase the value of the timeout to minimize disturbances related to the behavior defined by [session_probe]on_keepalive_timeout.
# The KeepAlive message is used to detect Session Probe unavailability. Without Session Probe, session monitoring will be minimal. No metadata will be collected.
# During the delay between sending a KeepAlive request and receiving the corresponding reply, Session Probe availability is indeterminate.
# (in milliseconds | min = 0, max = 60000)
#_advanced
# (acl config: proxy ⇐ session_probe:keepalive_timeout)
#keepalive_timeout = 5000

# This parameter allows us to choose the behavior of the RDP Proxy in case of losing the connection with Session Probe.
#   0: ignore and continue: Designed to minimize the impact on the user experience if the Session Probe is unstable. It should not be used when Session Probe is working well. An attacker can take advantage of this setting by simulating a Session Probe crash in order to bypass the surveillance.
#   1: disconnect user: Legacy behavior. It’s a choice that gives more security, but the impact on the user experience seems disproportionate. The RDP session can be closed (resulting in the permanent loss of all its unsaved elements) if the 'End disconnected session' parameter (or an equivalent setting at the RDS-level) is enabled.
#   2: freeze connection and wait: This is the recommended setting. User actions will be blocked until contact with the Session Probe (reply to KeepAlive message or something else) is resumed.
# (acl config: proxy ⇐ session_probe:on_keepalive_timeout)
#on_keepalive_timeout = 2

# The behavior of this parameter is different between the Desktop session and the RemoteApp session (RDS meaning). But in each case, the purpose of enabling this parameter is to not leave disconnected sessions in a state unusable by the RDP proxy.
# If enabled, Session Probe will automatically end the disconnected Desktop session. Otherwise, the RDP session and the applications it contains will remain active after user disconnection (unless a parameter defined at the RDS-level decides otherwise).
# The parameter in RemoteApp session (RDS meaning) does not cause the latter to be closed but a simple cleanup. However, this makes the session suitable for reuse.
# This parameter must be enabled for Web applications because an existing session with a running browser cannot be reused.
# It is also recommended to enable this parameter for connections in RemoteApp mode (RDS meaning) when [mod_rdp]use_session_probe_to_launch_remote_program is enabled. Because an existing Session Probe does not launch a startup program (a new Bastion application) when the RemoteApp session resumes.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ session_probe:end_disconnected_session)
#end_disconnected_session = 0

# If enabled, disconnected auto-deployed Application Driver session will automatically terminate by Session Probe.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable autodeployed Application Driver affinity
# (acl config: proxy ⇐ session_probe:enable_autodeployed_appdriver_affinity)
#enable_autodeployed_appdriver_affinity = 1

# This parameter allows you to enable the Windows-side logging of Session Probe.
# The generated files are located in the Windows user's temporary directory. These files can only be analyzed by the WALLIX team.
# This log does not help diagnose a launch problem. For this, you should instead use the debugging settings in the Configuration options.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ session_probe:enable_log)
#enable_log = 0

# This parameter enables or disables the Log files rotation for Windows-side logging of Session Probe.
# The Log files rotation helps reduce disk space consumption caused by logging. But the interesting information may be lost if the corresponding file is not retrieved in time.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ session_probe:enable_log_rotation)
#enable_log_rotation = 0

# Defines logging severity levels.
#   1: Fatal: Designates very severe error events that will presumably lead the application to abort.
#   2: Error: Designates error events that might still allow the application to continue running.
#   3: Info: Designates informational messages that highlight the progress of the application at coarse-grained level.
#   4: Warning: Designates potentially harmful situations.
#   5: Debug: Designates fine-grained informational events that are mostly useful to debug an application.
#   6: Detail: Designates finer-grained informational events than Debug.
#_advanced
# (acl config: proxy ⇐ session_probe:log_level)
#log_level = 5

# (Deprecated!)
# The period above which the disconnected Application session will be automatically closed by the Session Probe.
# 0 to disable timeout.
# (in milliseconds | min = 0, max = 172800000)
#_advanced
# (acl config: proxy ⇐ session_probe:disconnected_application_limit)
#disconnected_application_limit = 0

# The period above which the disconnected Desktop session will be automatically closed by the Session Probe.
# 0 to disable timeout.
# (in milliseconds | min = 0, max = 172800000)
#_advanced
# (acl config: proxy ⇐ session_probe:disconnected_session_limit)
#disconnected_session_limit = 0

# The period of user inactivity above which the session will be locked by the Session Probe.
# 0 to disable timeout.
# (in milliseconds | min = 0, max = 172800000)
#_advanced
# (acl config: proxy ⇐ session_probe:idle_session_limit)
#idle_session_limit = 0

# The additional period given to the device to make Clipboard redirection available.
# This parameter is effective only if [session_probe]use_smart_launcher is enabled.
# If we see the message "Clipboard Virtual Channel is unavailable" in the Bastion’s syslog and we are sure that this virtual channel is allowed on the device (confirmed by a direct connection test for example), we probably need to use this parameter.
# (in milliseconds)
#_advanced
# (acl config: proxy ⇐ session_probe:smart_launcher_clipboard_initialization_delay)
#smart_launcher_clipboard_initialization_delay = 2000

# For under-performing devices.
# The extra time given to the device before starting the Session Probe launch sequence.
# This parameter is effective only if [session_probe]use_smart_launcher is enabled.
# This parameter can be useful when (with Launch mask disabled) Windows Explorer is not immediately visible when the RDP session is opened.
# (in milliseconds)
#_advanced
# (acl config: proxy ⇐ session_probe:smart_launcher_start_delay)
#smart_launcher_start_delay = 0

# The delay between two simulated keystrokes during the Session Probe launch sequence execution.
# This parameter is effective only if [session_probe]use_smart_launcher is enabled.
# This parameter may help if the Session Probe launch failure is caused by network slowness or device under-performance.
# This parameter is usually used together with the [session_probe]smart_launcher_short_delay.
# (in milliseconds)
#_advanced
# (acl config: proxy ⇐ session_probe:smart_launcher_long_delay)
#smart_launcher_long_delay = 500

# The delay between two steps of the same simulated keystrokes during the Session Probe launch sequence execution.
# This parameter is effective only if [session_probe]use_smart_launcher is enabled.
# This parameter may help if the Session Probe launch failure is caused by network slowness or device under-performance.
# This parameter is usually used together with the [session_probe]smart_launcher_long_delay.
# (in milliseconds)
#_advanced
# (acl config: proxy ⇐ session_probe:smart_launcher_short_delay)
#smart_launcher_short_delay = 50

# Allow sufficient time for the RDP client (Access Manager) to respond to the Clipboard virtual channel initialization message. Otherwise, the time granted to the RDP client (Access Manager or another) for Clipboard virtual channel initialization will be defined by the [session_probe]smart_launcher_clipboard_initialization_delay.
# This parameter is effective only if [session_probe]use_smart_launcher is enabled and the RDP client is Access Manager.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Enable Smart launcher with AM affinity
# (acl config: proxy ⇐ session_probe:smart_launcher_enable_wabam_affinity)
#smart_launcher_enable_wabam_affinity = 1

# The time interval between the detection of an error (example: a refusal by the target of the redirected drive) and the actual abandonment of the Session Probe launch.
# The purpose of this parameter is to give the target time to gracefully stop some ongoing processing.
# It is strongly recommended to keep the default value of this parameter.
# (in milliseconds | min = 0, max = 300000)
#_advanced
# (acl config: proxy ⇐ session_probe:launcher_abort_delay)
#launcher_abort_delay = 2000

# This parameter enables or disables the crash dump generation when the Session Probe encounters a fatal error.
# The crash dump file is useful for post-modem debugging. It is not designed for normal use.
# The generated files are located in the Windows user's temporary directory. These files can only be analyzed by the WALLIX team.
# There is no rotation mechanism to limit the number of dump files produced. Extended activation of this parameter can quickly exhaust disk space.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ session_probe:enable_crash_dump)
#enable_crash_dump = 0

# Use only if you see unusually high consumption of system object handles by the Session Probe.
# The Session Probe will sabotage and then restart it-self if it consumes more handles than what is defined by this parameter.
# A value of 0 disables this feature.
# This feature can cause the session to be disconnected if the value of the [session_probe]on_keepalive_timeout is set to 1 (Disconnect user).
# If [session_probe]allow_multiple_handshake is disabled, restarting the Session Probe will cause the session to disconnect.
# (min = 0, max = 1000)
#_advanced
# (acl config: proxy ⇐ session_probe:handle_usage_limit)
#handle_usage_limit = 0

# Use only if you see unusually high consumption of memory by the Session Probe.
# The Session Probe will sabotage and then restart it-self if it consumes more memory than what is defined by this parameter.
# A value of 0 disables this feature.
# This feature can cause the session to be disconnected if the value of the [session_probe]on_keepalive_timeout is set to 1 (Disconnect user).
# If [session_probe]allow_multiple_handshake is disabled, restarting the Session Probe will cause the session to disconnect.
# (min = 0, max = 200000000)
#_advanced
# (acl config: proxy ⇐ session_probe:memory_usage_limit)
#memory_usage_limit = 0

# This debugging feature was created to determine the cause of high CPU consumption by Session Probe in certain environments.
# As a percentage, the effective alarm threshold is calculated in relation to the reference consumption determined at the start of the program execution. The alarm is deactivated if this value of parameter is less than 200 (200% of reference consumption).
# When CPU consumption exceeds the allowed limit, debugging information can be collected (if the Windows-side logging is enabled), then Session Probe will sabotage. Additional behavior is defined by [session_probe]cpu_usage_alarm_action.
# (min = 0, max = 10000)
#_advanced
#_display_name=CPU usage alarm threshold
# (acl config: proxy ⇐ session_probe:cpu_usage_alarm_threshold)
#cpu_usage_alarm_threshold = 0

# Additional behavior when CPU consumption exceeds what is allowed. Refer to the [session_probe]cpu_usage_alarm_threshold.
#   0: Restart the Session Probe. May result in session disconnection due to loss of KeepAlive messages! Refer to 'On keepalive timeout' parameter of current section and 'Allow multiple handshakes' parameter of 'Configuration options'.
#   1: Stop the Session Probe. May result in session disconnection due to loss of KeepAlive messages! Refer to 'On keepalive timeout' parameter of current section.
#_advanced
#_display_name=CPU usage alarm action
# (acl config: proxy ⇐ session_probe:cpu_usage_alarm_action)
#cpu_usage_alarm_action = 0

# For application session only.
# The delay between the launch of the application and the start of End of session check.
# Sometimes an application takes a long time to create its window. If the End of session check is start too early, the Session Probe may mistakenly conclude that there is no longer any active process in the session. And without active processes, the application session will be logged off by the Session Probe.
# 'End of session check delay time' allow you to delay the start of End of session check in order to give the application the time to create its window.
# (in milliseconds | min = 0, max = 60000)
#_advanced
# (acl config: proxy ⇐ session_probe:end_of_session_check_delay_time)
#end_of_session_check_delay_time = 0

# For application session only.
# If enabled, during the End of session check, the processes that do not have a visible window will not be counted as active processes of the session. Without active processes, the application session will be logged off by the Session Probe.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Ignore UI less processes during end of session check
# (acl config: proxy ⇐ session_probe:ignore_ui_less_processes_during_end_of_session_check)
#ignore_ui_less_processes_during_end_of_session_check = 1

# This parameter is used to provide the list of (comma-separated) system processes that can be run in the session.
# Ex.: dllhos.exe,TSTheme.exe
# Unlike user processes, system processes do not keep the session open. A session with no user process will be automatically closed by Session Probe after starting the End of session check.
# (acl config: proxy ⇐ session_probe:extra_system_processes)
#extra_system_processes = 

# This parameter concerns the functionality of the Password field detection performed by the Session Probe. This detection is necessary to avoid logging the text entered in the password fields as metadata of session (also known as Session log).
# Unfortunately, the detection does not work with applications developed in Java, Flash, etc. In order to work around the problem, we will treat the windows of these applications as input fields of unknown type. Therefore, the text entered in these will not be included in the session’s metadata.
# One of the specifics of these applications is that their main windows do not have any child window from point of view of WIN32 API. Activating this parameter allows this property to be used to detect applications developed in Java or Flash.
# Refer to the [session_log]keyboard_input_masking_level.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ session_probe:childless_window_as_unidentified_input_field)
#childless_window_as_unidentified_input_field = 1

# Comma-separated process names. (Ex.: chrome.exe,ngf.exe)
# This parameter concerns the functionality of the Password field detection performed by the Session Probe. This detection is necessary to avoid logging the text entered in the password fields as metadata of session (also known as Session log).
# Unfortunately, the detection is not infallible. In order to work around the problem, we will treat the windows of these applications as input fields of unknown type. Therefore, the text entered in these will not be included in the session’s metadata.
# This parameter is used to provide the list of processes whose windows are considered as input fields of unknown type.
# Refer to the [session_log]keyboard_input_masking_level.
# (acl config: proxy ⇐ session_probe:windows_of_these_applications_as_unidentified_input_field)
#windows_of_these_applications_as_unidentified_input_field = 

# This parameter is used when resuming a session hosting a existing Session Probe.
# If enabled, the Session Probe will activate or deactivate features according to the value of [session_probe]disabled_features received when resuming its host session. Otherwise, the Session Probe will keep the same set of features that were used during the previous connection.
# It is recommended to keep the default value of this parameter.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ session_probe:update_disabled_features)
#update_disabled_features = 1

# This parameter was created to work around some compatibility issues and to limit the CPU load that the Session Probe process causes.
# If 'Java Acccess Bridge' feature is disabled, data entered in the password field of Java applications may be visible in the metadata.
# For more information, refer to [session_log]keyboard_input_masking_level.
# For more information, also refer to [session_probe]childless_window_as_unidentified_input_field and [session_probe]windows_of_these_applications_as_unidentified_input_field.It is not recommended to deactivate 'MS Active Accessibility' and 'MS UI Automation' at the same time. This configuration will lead to the loss of detection of password input fields. Entries in these fields will be visible as plain text in the session metadata.
#   0x000: none
#   0x001: disable Java Access Bridge. General user activity monitoring in the Java applications (including detection of password fields).
#   0x002: disable MS Active Accessbility. General user activity monitoring (including detection of password fields). (legacy API)
#   0x004: disable MS UI Automation. General user activity monitoring (including detection of password fields). (new API)
#   0x010: disable Inspect Edge location URL. Basic web navigation monitoring.
#   0x020: disable Inspect Chrome Address/Search bar. Basic web navigation monitoring.
#   0x040: disable Inspect Firefox Address/Search bar. Basic web navigation monitoring.
#   0x080: disable Monitor Internet Explorer event. Advanced web navigation monitoring.
#   0x100: disable Inspect group membership of user. User identity monitoring.
# 
# Note: values can be added (disable all: 0x1 + 0x2 + 0x4 + 0x10 + 0x20 + 0x40 + 0x80 + 0x100 = 0x1f7)
#_advanced
# (acl config: proxy ⇐ session_probe:disabled_features)
#disabled_features = 352

# This parameter has no effect on the device without BestSafe.
# Is enabled, Session Probe relies on BestSafe to perform the detection of application launches and the detection of outgoing connections.
# BestSafe has more efficient mechanisms in these tasks than Session Probe.
# For more information, refer to [session_probe]outbound_connection_monitoring_rules and [session_probe]process_monitoring_rules.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable BestSafe interaction
# (acl config: proxy ⇐ session_probe:enable_bestsafe_interaction)
#enable_bestsafe_interaction = 0

# This parameter has no effect on the device without BestSafe.
# BestSafe interaction must be enabled. Refer to [session_probe]enable_bestsafe_interaction.
# This parameter allows you to choose the behavior of the RDP Proxy in case of detection of Windows account manipulation.
# Detectable account manipulations are the creation, deletion of a Windows account, and the addition and deletion of an account from a Windows user group.
#   0: allow: User action will be accepted.
#   1: notify: (Same thing as 'allow'.)
#   2: deny: User action will be rejected.
# (acl config: proxy ⇐ session_probe:on_account_manipulation)
#on_account_manipulation = 0

# This parameter is used to indicate the name of an environment variable, to be set on the Windows device, and pointed to a directory (on the device) that can be used to store and start the Session Probe. The environment variable must be available in the Windows user session.
# The environment variable name is limited to 3 characters or less.
# By default, the Session Probe will be stored and started from the temporary directory of Windows user.
# This parameter is useful if a GPO prevents Session Probe from starting from the Windows user's temporary directory.
# (maxlen = 3)
#_advanced
# (acl config: proxy ⇐ session_probe:alternate_directory_environment_variable)
#alternate_directory_environment_variable = 

# If enabled, the session, once disconnected, can be resumed by another Bastion user.
# Except in special cases, this is usually a security problem.
# By default, a session can only be resumed by the Bastion user who created it.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ session_probe:public_session)
#public_session = 0

# This parameter is used to provide the list of (comma-separated) rules used to monitor outgoing connections created in the session.
# (Ex. IPv4 addresses: $deny:192.168.0.0/24:5900,$allow:192.168.0.110:21)
# (Ex. IPv6 addresses: $deny:2001:0db8:85a3:0000:0000:8a2e:0370:7334:3389,$allow:[20D1:0:3238:DFE1:63::FEFB]:21)
# (Ex. hostname can be used to resolve to both IPv4 and IPv6 addresses: $allow:host.domain.net:3389)
# (Ex. for backwards compatibility only: 10.1.0.0/16:22)
# A global rule that matches all IP addresses/Ports such as "0.0.0.0/0:*" is highly unrecommended.
# Session Probe must be enabled to use this feature.
# BestSafe can be used to perform detection of outgoing connections created in the session. Refer to [session_probe]enable_bestsafe_interaction.
# (acl config: proxy ⇐ session_probe:outbound_connection_monitoring_rules)
#outbound_connection_monitoring_rules = 

# This parameter is used to provide the list of (comma-separated) rules used to monitor the execution of processes in the session.
# (Ex.: $deny:taskmgr.exe)
# @ = All child processes of (Bastion) application (Ex.: $deny:@)
# BestSafe can be used to perform detection of process launched in the session. Refer to [session_probe]enable_bestsafe_interaction.
# (acl config: proxy ⇐ session_probe:process_monitoring_rules)
#process_monitoring_rules = 

# If enabled, a string of random characters will be added to the name of the Session Probe executable.
# The result could be: SesProbe-5420.exe
# Some other features automatically enable customization of the Session Probe executable name. Application Driver auto-deployment for example.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#customize_executable_name = 0

# If enabled, the RDP Proxy accepts to perform the handshake several times during the same RDP session. Otherwise, any new handshake attempt will interrupt the current session with the display of an alert message.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=Allow multiple handshakes
#allow_multiple_handshake = 0

# If disabled, the RDP proxy disconnects from the session when the Session Probe reports that the session is about to close (old behavior).
# The new session end procedure (freeze and wait) prevents another connection from resuming a session that is close to end-of-life.
# (type: boolean (0/no/false or 1/yes/true))
#at_end_of_session_freeze_connection_and_wait = 1

# (type: boolean (0/no/false or 1/yes/true))
#enable_cleaner = 1

# (type: boolean (0/no/false or 1/yes/true))
#clipboard_based_launcher_reset_keyboard_status = 1

#   0: windows management instrumentation: Get command-line of processes via Windows Management Instrumentation. (Legacy method)
#   1: windows internals: Calling internal system APIs to get the process command line. (More efficient but less stable)
#   2: both: First use internal system APIs call, if that fails, use Windows Management Instrumentation method.
#_advanced
# (acl config: proxy ⇐ session_probe:process_command_line_retrieve_method)
#process_command_line_retrieve_method = 2

# Time between two polling performed by Session Probe.
# The parameter is created to adapt the CPU consumption to the performance of the Windows device.
# The longer this interval, the less detailed the session metadata collection and the lower the CPU consumption.
# (in milliseconds | min = 300, max = 2000)
#_advanced
# (acl config: proxy ⇐ session_probe:periodic_task_run_interval)
#periodic_task_run_interval = 500

# If enabled, Session Probe activity will be minimized when the user is disconnected from the session. No metadata will be collected during this time.
# The purpose of this behavior is to optimize CPU consumption.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ session_probe:pause_if_session_is_disconnected)
#pause_if_session_is_disconnected = 0

# If enabled, Session Probe will monitor its own system resource consumption.
# This feature increases CPU consumption.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ session_probe:monitor_own_resources_consumption)
#monitor_own_resources_consumption = 0

[server_cert]

# Keep known target server certificates on Bastion
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ server_cert:server_cert_store)
#server_cert_store = 1

# Behavior of certificates check.
#   0: fails if certificates do not match or are missing.
#   1: fails if certificate does not match, succeeds if no known certificate.
#   2: succeeds if certificates exist (not checked), fails if missing.
#   3: always succeed.
# System errors like FS access rights issues or certificate decode are always check errors leading to connection rejection.
# (acl config: proxy ⇐ server_cert:server_cert_check)
#server_cert_check = 1

# Warn if check allow connexion to target server.
#   0x0: nobody
#   0x1: SIEM: message sent to SIEM
# 
# Note: values can be added (enable all: 0x1 = 0x1)
#_advanced
# (acl config: proxy ⇐ server_cert:server_access_allowed_message)
#server_access_allowed_message = 0

# Warn that new target server certificate file was created.
#   0x0: nobody
#   0x1: SIEM: message sent to SIEM
# 
# Note: values can be added (enable all: 0x1 = 0x1)
#_advanced
# (acl config: proxy ⇐ server_cert:server_cert_create_message)
#server_cert_create_message = 1

# Warn that target server certificate file was successfully checked.
#   0x0: nobody
#   0x1: SIEM: message sent to SIEM
# 
# Note: values can be added (enable all: 0x1 = 0x1)
#_advanced
# (acl config: proxy ⇐ server_cert:server_cert_success_message)
#server_cert_success_message = 0

# Warn that target server certificate file checking failed.
#   0x0: nobody
#   0x1: SIEM: message sent to SIEM
# 
# Note: values can be added (enable all: 0x1 = 0x1)
#_advanced
# (acl config: proxy ⇐ server_cert:server_cert_failure_message)
#server_cert_failure_message = 1

# Warn that target server certificate check raised some internal error.
#   0x0: nobody
#   0x1: SIEM: message sent to SIEM
# 
# Note: values can be added (enable all: 0x1 = 0x1)
#error_message = 1

[mod_vnc]

# Check this option to enable the clipboard upload (from client to target server).
# This only supports text data clipboard (not files).
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ clipboard_up)
#clipboard_up = 0

# Check this option to enable the clipboard download (from target server to client).
# This only supports text data clipboard (not files).
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ clipboard_down)
#clipboard_down = 0

# Sets additional graphics encoding types that will be negotiated with the VNC target server:
#   2: RRE
#   5: HEXTILE
#   16: ZRLE
# (values are comma-separated)
#_advanced
#encodings = 

# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ mod_vnc:support_cursor_pseudo_encoding)
#support_cursor_pseudo_encoding = 1

# VNC target server clipboard text data encoding type.
# values: utf-8, latin1
#_advanced
# (acl config: proxy ⇐ vnc_server_clipboard_encoding_type)
#server_clipboard_encoding_type = latin1

# The RDP clipboard is based on a token that indicates who owns data between target server and client. However, some RDP clients, such as FreeRDP, always appropriate this token. This conflicts with VNC, which also appropriates this token, causing clipboard data to be sent in loops.
# This option indicates the strategy to adopt in such situations.
#   0: delayed: Clipboard processing is deferred and, if necessary, the token is left with the client.
#   1: duplicated: When 2 identical requests are received, the second is ignored. This can block clipboard data reception until a clipboard event is triggered on the target server when the client clipboard is blocked, and vice versa.
#   2: continued: No special processing is done, the proxy always responds immediately.
#_advanced
# (acl config: proxy ⇐ vnc_bogus_clipboard_infinite_loop)
#bogus_clipboard_infinite_loop = 0

# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Server is MacOS
# (acl config: proxy ⇐ mod_vnc:server_is_macos)
#server_is_macos = 0

# When disabled, Ctrl + Alt becomes AltGr (Windows behavior).
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Server Unix alt
# (acl config: proxy ⇐ mod_vnc:server_unix_alt)
#server_unix_alt = 0

# Enable target connection on IPv6.
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable IPv6
# (acl config: proxy ⇐ mod_vnc:enable_ipv6)
#enable_ipv6 = 1

# Minimal incoming TLS level 0=TLSv1, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3
# (min = 0)
#_display_name=TLS min level
# (acl config: proxy ⇐ mod_vnc:tls_min_level)
#tls_min_level = 2

# Maximal incoming TLS level 0=no restriction, 1=TLSv1.1, 2=TLSv1.2, 3=TLSv1.3
# (min = 0)
#_display_name=TLS max level
# (acl config: proxy ⇐ mod_vnc:tls_max_level)
#tls_max_level = 0

# TLSv1.2 and below additional ciphers supported.
# Empty to apply system-wide configuration (SSL security level 2), ALL for support of all ciphers to ensure highest compatibility with target servers.
# The format used is described on this page: https://www.openssl.org/docs/man3.1/man1/openssl-ciphers.html#CIPHER-LIST-FORMAT
# (acl config: proxy ⇐ mod_vnc:cipher_string)
#cipher_string = ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES128-GCM-SHA256

# Allow TLS legacy insecure renegotiation to unpatched target servers.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#_display_name=TLS enable legacy server
# (acl config: proxy ⇐ mod_vnc:tls_enable_legacy_server)
#tls_enable_legacy_server = 0

# Configure the available TLSv1.3 ciphersuites.
# Empty to apply system-wide configuration.
# The format used is described in the third paragraph of this page: https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_ciphersuites.html#DESCRIPTION
#_display_name=TLS 1.3 cipher suites
# (acl config: proxy ⇐ mod_vnc:tls_1_3_ciphersuites)
#tls_1_3_ciphersuites = 

# Configure the supported key exchange groups.
# Empty to apply system-wide configuration.
# The format used is described in this page: https://www.openssl.org/docs/man3.2/man3/SSL_CONF_cmd.html#groups-groups
#_display_name=TLS key exchange groups
# (acl config: proxy ⇐ mod_vnc:tls_key_exchange_groups)
#tls_key_exchange_groups = P-256:P-384:P-521:ffdhe3072:ffdhe4096:ffdhe6144:ffdhe8192

# Show in the logs the common cipher list supported by client and target server.
# ⚠ Only for debugging purposes.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ mod_vnc:show_common_cipher_list)
#show_common_cipher_list = 0

# ⚠ The use of this feature is not recommended!
# 
# When specified, force the proxy to use a specific authentication method. If this method is not supported by the target server, the connection will not be made.
#   - noauth
#   - vncauth
#   - mslogon
#   - mslogoniiauth
#   - ultravnc_dsm_old
#   - ultravnc_dsm_new
#   - tlsnone
#   - tlsvnc
#   - tlsplain
#   - x509none
#   - x509vnc
#   - x509plain
#_advanced
# (acl config: proxy ⇐ mod_vnc:force_authentication_method)
#force_authentication_method = 

[session_log]

# Saves session logs to a .log file.
# The format is a date followed by one or more key="value" separated by a space on the same line.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ session_log:enable_session_log_file)
#enable_session_log_file = 1

# Writes session logs to syslog and define its format.
#   0x0: disabled
#   0x1: SIEM
#   0x2: ArcSight
# 
# Note: values can be added (enable all: 0x1 + 0x2 = 0x3)
#enable_syslog_format = 1

# Classification of input data is performed using Session Probe.
# Without Session Probe, all the texts entered are considered unidentified.
#   0: Keyboard inputs are not masked.
#   1: Only passwords are masked.
#   2: Passwords and unidentified texts are masked.
#   3: Keyboard inputs are not logged.
# (acl config: proxy ⇐ session_log:keyboard_input_masking_level)
#keyboard_input_masking_level = 2

[ocr]

# Selects the OCR (Optical Character Recognition) version used to detect title bars when Session Probe is not running.
# Version 1 is a bit faster, but has a higher failure rate in character recognition.
#   1: v1
#   2: v2
#version = 2

#   latin: Recognizes Latin characters
#   cyrillic: Recognizes Latin and Cyrillic characters
#locale = latin

# Time interval between two analyses.
# A value too low will affect session reactivity.
# (in 1/100 seconds)
#_advanced
#interval = 100

# Checks shape and color to determine if the text is on a title bar.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#on_title_bar_only = 1

# Expressed in percentage,
#   0   - all of characters need be recognized
#   100 - accept all results
# (min = 0, max = 100)
#_advanced
#max_unrecog_char_rate = 40

[capture]

# (maxlen = 4096)
# (acl config: proxy ⇐ hash_path)
#hash_path = )gen_config_ini" << (app_path(AppPath::Hash)) << R"gen_config_ini(

# (maxlen = 4096)
# (acl config: proxy ⇐ record_tmp_path)
#record_tmp_path = )gen_config_ini" << (app_path(AppPath::RecordTmp)) << R"gen_config_ini(

# (maxlen = 4096)
# (acl config: proxy ⇐ record_path)
#record_path = )gen_config_ini" << (app_path(AppPath::Record)) << R"gen_config_ini(

# Specifies the type of data to be captured:
#   0x0: none
#   0x1: png
#   0x2: wrm: Session recording file.
#   0x8: ocr
# 
# Note: values can be added (enable all: 0x1 + 0x2 + 0x8 = 0xb)
#_advanced
#capture_flags = 11

# Disable keyboard log:
# (See also [session_log]keyboard_input_masking_level)
#   0x0: none
#   0x1: disable keyboard log in session log
#   0x2: disable keyboard log in recorded sessions
# 
# Note: values can be added (disable all: 0x1 + 0x2 = 0x3)
#_advanced
# (acl config: proxy ⇐ capture:disable_keyboard_log)
#disable_keyboard_log = 0

# Disable clipboard log:
#   0x0: none
#   0x1: disable clipboard log in recorded sessions
#   0x2: disable clipboard log in recorded meta
# 
# Note: values can be added (disable all: 0x1 + 0x2 = 0x3)
#_advanced
#disable_clipboard_log = 0

# Disable (redirected) file system log:
#   0x0: none
#   0x1: disable (redirected) file system log in recorded sessions
#   0x2: disable (redirected) file system log in recorded meta
# 
# Note: values can be added (disable all: 0x1 + 0x2 = 0x3)
#_advanced
#disable_file_system_log = 0

# Time between two .wrm recording files.
# ⚠ A value that is too small increases the disk space required for recordings.
# (in seconds)
#_advanced
#wrm_break_interval = 600

# Color depth for the Session Recording file (.wrm):
#   0: 24-bit
#   1: 16-bit
#_advanced
#wrm_color_depth_selection_strategy = 1

# Compression method of the Session Recording file (.wrm):
#   0: no compression
#   1: GZip: Files are better compressed, but this takes more time and CPU load.
#   2: Snappy: Faster than GZip, but files are less compressed.
#_advanced
#wrm_compression_algorithm = 1

# Allow to control permissions on recorded files.
# (in octal or symbolic mode format (as chmod Linux command))
#file_permissions = 440

[audit]

# Show keyboard input events in the meta file.
# (See also [session_log]keyboard_input_masking_level for RDP and [capture]disable_keyboard_log for VNC and RDP)
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#enable_keyboard_log = 1

# The maximum time between two videos when no title bar is detected.
# (in seconds)
#_advanced
#video_break_interval = 604800

# Maximum number of images per second for video generation.
# A higher value will produce smoother videos, but the file weight is higher and the generation time longer.
# (min = 1, max = 120)
#_advanced
#video_frame_rate = 5

# In the generated video of the session record traces, remove the top left banner with the timestamp.
# Can slightly speed up the video generation.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#video_notimestamp = 0

# Video codec used for video generation.
#video_codec = mp4

# FFmpeg options for video codec. See https://trac.ffmpeg.org/wiki/Encode/H.264
# ⚠ Some browsers and video decoders don't support crf=0
#_advanced
#_display_name=FFmpeg options
#ffmpeg_options = crf=35 preset=superfast

#   0: disable: When replaying the session video, the content of the RDP viewer matches the size of the client's desktop.
#   1: v1: When replaying the session video, the content of the RDP viewer is restricted to the greatest area covered by the application during the session.
#   2: v2: When replaying the session video, the content of the RDP viewer is fully covered by the size of the greatest application window during the session.
#smart_video_cropping = 2

# Checking this option will allow to play a video with corrupted Bitmap Update.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#play_video_with_corrupted_bitmap = 0

# Allow to control permissions on video files.
# (in octal or symbolic mode format (as chmod Linux command))
#file_permissions = 440

# Allow real-time view (4 eyes) without session recording enabled in the authorization.
# (type: boolean (0/no/false or 1/yes/true))
#allow_rt_without_recording = 0

# Frame interval for 4eyes. A value lower than 6 will have no visible effect.
# (in milliseconds)
#rt_png_interval = 1000

# Number of png captures to keep.
# (min = 0)
#rt_png_limit = 5

# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Use Redis
#use_redis = 1

# (in milliseconds)
#_display_name=Redis timeout
#redis_timeout = 500

[file_verification]

#socket_path = )gen_config_ini" << (REDEMPTION_CONFIG_VALIDATOR_PATH) << R"gen_config_ini(

# Enable use of ICAP service for file verification on upload.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ file_verification:enable_up)
#enable_up = 0

# Enable use of ICAP service for file verification on download.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ file_verification:enable_down)
#enable_down = 0

# Verify text data via clipboard from the client to the target server.
# File verification on upload must be enabled via the Enable up option.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ file_verification:clipboard_text_up)
#clipboard_text_up = 0

# Verify text data via clipboard from the target server to the client
# File verification on download must be enabled via the Enable down option.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ file_verification:clipboard_text_down)
#clipboard_text_down = 0

# Block file transfer from the client to the target server on invalid file verification.
# File verification on upload must be enabled via the Enable up option.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ file_verification:block_invalid_file_up)
#block_invalid_file_up = 0

# Block file transfer from the target server to the client on invalid file verification.
# File verification on download must be enabled via the Enable down option.
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ file_verification:block_invalid_file_down)
#block_invalid_file_down = 0

# Block text transfer from the client to the target server on invalid text verification.
# Text verification on upload must be enabled via the Clipboard text up option.
# (type: boolean (0/no/false or 1/yes/true))
#block_invalid_clipboard_text_up = 0

# Block text transfer from the target server to the client on invalid text verification.
# Text verification on download must be enabled via the Clipboard text down option.
# (type: boolean (0/no/false or 1/yes/true))
#block_invalid_clipboard_text_down = 0

# Log the files and clipboard texts that are verified and accepted. By default, only those rejected are logged.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
# (acl config: proxy ⇐ file_verification:log_if_accepted)
#log_if_accepted = 1

# ⚠ This value affects the RAM used by the session.
# 
# If option Block invalid file (up or down) is enabled, automatically reject file with greater filesize.
# (in megabytes)
#_advanced
# (acl config: proxy ⇐ file_verification:max_file_size_rejected)
#max_file_size_rejected = 256

# Temporary path used when files take up too much memory.
# (maxlen = 4096)
#tmpdir = /tmp/

[file_storage]

# Enable storage of transferred files (via RDP Clipboard).
# ⚠ Saving files can take up a lot of disk space.
#   never: Never store transferred files.
#   always: Always store transferred files.
#   on_invalid_verification: Transferred files are stored only if file verification is invalid. File verification by ICAP service must be enabled (in section file_verification).
# (acl config: proxy ⇐ file_storage:store_file)
#store_file = never

[websocket]

# Enable websocket protocol (ws or wss with use_tls=1).
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Enable WebSocket
#enable_websocket = 0

# Use TLS with websocket (wss).
# (type: boolean (0/no/false or 1/yes/true))
#_display_name=Use TLS
#use_tls = 1

# ${addr}:${port} or ${port} or ${unix_socket_path}
#listen_address = :3390

[internal_mod]

# Allow separate target and login entries by enabling the edit target field on the login page.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#enable_target_field = 1

# List of keyboard layouts available by the internal pages button located at bottom left of some internal pages (login, selector, etc).
# (values are comma-separated)
# <details><summary>Possible values in alphabetical order:</summary><table><tr><th>Value</th><th>Windows 11 name (informative)</th></tr><tr><td>ar-SA.101</td><td>Arabic (101)</td></tr><tr><td>ar-SA.102</td><td>Arabic (102)</td></tr><tr><td>ar-SA.102-azerty</td><td>Arabic (102) AZERTY</td></tr><tr><td>as</td><td>Assamese - INSCRIPT</td></tr><tr><td>az-Cyrl</td><td>Azerbaijani Cyrillic</td></tr><tr><td>az-Latn</td><td>Azerbaijani Latin</td></tr><tr><td>az-Latn.standard</td><td>Azerbaijani (Standard)</td></tr><tr><td>ba-Cyrl</td><td>Bashkir</td></tr><tr><td>be</td><td>Belarusian</td></tr><tr><td>bg-BG</td><td>Bulgarian</td></tr><tr><td>bg-BG.latin</td><td>Bulgarian (Latin)</td></tr><tr><td>bg.phonetic</td><td>Bulgarian (Phonetic)</td></tr><tr><td>bg.phonetic.trad</td><td>Bulgarian (Phonetic Traditional)</td></tr><tr><td>bg.typewriter</td><td>Bulgarian (Typewriter)</td></tr><tr><td>bn-IN</td><td>Bangla</td></tr><tr><td>bn-IN.inscript</td><td>Bangla - INSCRIPT</td></tr><tr><td>bn-IN.legacy</td><td>Bangla - INSCRIPT (Legacy)</td></tr><tr><td>bo-Tibt</td><td>Tibetan (PRC)</td></tr><tr><td>bo-Tibt.updated</td><td>Tibetan (PRC) - Updated</td></tr><tr><td>bs-Cy</td><td>Bosnian (Cyrillic)</td></tr><tr><td>bug-Bugi</td><td>Buginese</td></tr><tr><td>bépo</td><td>French (Standard, BÉPO)</td></tr><tr><td>chr-Cher</td><td>Cherokee Nation</td></tr><tr><td>chr-Cher.phonetic</td><td>Cherokee Phonetic</td></tr><tr><td>cs-CZ</td><td>Czech</td></tr><tr><td>cs-CZ.programmers</td><td>Czech Programmers</td></tr><tr><td>cs-CZ.qwerty</td><td>Czech (QWERTY)</td></tr><tr><td>cy-GB</td><td>United Kingdom Extended</td></tr><tr><td>da-DK</td><td>Danish</td></tr><tr><td>de-CH</td><td>Swiss German</td></tr><tr><td>de-DE</td><td>German</td></tr><tr><td>de-DE.ex1</td><td>German Extended (E1)</td></tr><tr><td>de-DE.ex2</td><td>German Extended (E2)</td></tr><tr><td>de-DE.ibm</td><td>German (IBM)</td></tr><tr><td>dv.phonetic</td><td>Divehi Phonetic</td></tr><tr><td>dv.typewriter</td><td>Divehi Typewriter</td></tr><tr><td>dz</td><td>Dzongkha</td></tr><tr><td>el-GR</td><td>Greek</td></tr><tr><td>el-GR.220</td><td>Greek (220)</td></tr><tr><td>el-GR.220_latin</td><td>Greek (220) Latin</td></tr><tr><td>el-GR.319</td><td>Greek (319)</td></tr><tr><td>el-GR.319_latin</td><td>Greek (319) Latin</td></tr><tr><td>el-GR.latin</td><td>Greek Latin</td></tr><tr><td>el-GR.polytonic</td><td>Greek Polytonic</td></tr><tr><td>en-CA.fr</td><td>Canadian French</td></tr><tr><td>en-CA.multilingual</td><td>Canadian Multilingual Standard</td></tr><tr><td>en-GB</td><td>United Kingdom</td></tr><tr><td>en-IE</td><td>Scottish Gaelic</td></tr><tr><td>en-IE.irish</td><td>Irish</td></tr><tr><td>en-IN</td><td>English (India)</td></tr><tr><td>en-NZ</td><td>NZ Aotearoa</td></tr><tr><td>en-US</td><td>US</td></tr><tr><td>en-US.arabic.238L</td><td>US English Table for IBM Arabic 238_L</td></tr><tr><td>en-US.colemak</td><td>Colemak</td></tr><tr><td>en-US.dvorak</td><td>United States-Dvorak</td></tr><tr><td>en-US.dvorak_left</td><td>United States-Dvorak for left hand</td></tr><tr><td>en-US.dvorak_right</td><td>United States-Dvorak for right hand</td></tr><tr><td>en-US.international</td><td>United States-International</td></tr><tr><td>es-ES</td><td>Spanish</td></tr><tr><td>es-ES.variation</td><td>Spanish Variation</td></tr><tr><td>es-MX</td><td>Latin American</td></tr><tr><td>et-EE</td><td>Estonian</td></tr><tr><td>ett-Ital</td><td>Old Italic</td></tr><tr><td>fa</td><td>Persian</td></tr><tr><td>fa.standard</td><td>Persian (Standard)</td></tr><tr><td>ff-Adlm</td><td>ADLaM</td></tr><tr><td>fi-FI.finnish</td><td>Finnish</td></tr><tr><td>fi-SE</td><td>Finnish with Sami</td></tr><tr><td>fo-FO</td><td>Faeroese</td></tr><tr><td>fr-BE</td><td>Belgian (Comma)</td></tr><tr><td>fr-BE.fr</td><td>Belgian French</td></tr><tr><td>fr-CA</td><td>Canadian French (Legacy)</td></tr><tr><td>fr-CH</td><td>Swiss French</td></tr><tr><td>fr-FR</td><td>French (Legacy, AZERTY)</td></tr><tr><td>fr-FR.standard</td><td>French (Standard, AZERTY)</td></tr><tr><td>gem-Runr</td><td>Futhark</td></tr><tr><td>gn</td><td>Guarani</td></tr><tr><td>got-Goth</td><td>Gothic</td></tr><tr><td>gu</td><td>Gujarati</td></tr><tr><td>ha-Latn</td><td>Hausa</td></tr><tr><td>haw-Latn</td><td>Hawaiian</td></tr><tr><td>he</td><td>Hebrew</td></tr><tr><td>he-IL</td><td>Hebrew (Standard, 2018)</td></tr><tr><td>he.standard</td><td>Hebrew (Standard)</td></tr><tr><td>hi</td><td>Hindi Traditional</td></tr><tr><td>hi.inscript</td><td>Devanagari - INSCRIPT</td></tr><tr><td>hr-HR</td><td>Croatian</td></tr><tr><td>hsb</td><td>Sorbian Standard</td></tr><tr><td>hsb.ex</td><td>Sorbian Extended</td></tr><tr><td>hsb.legacy</td><td>Sorbian Standard (Legacy)</td></tr><tr><td>hu-HU</td><td>Hungarian</td></tr><tr><td>hu-HU.101-key</td><td>Hungarian 101-key</td></tr><tr><td>hy.east</td><td>Armenian Eastern (Legacy)</td></tr><tr><td>hy.phonetic</td><td>Armenian Phonetic</td></tr><tr><td>hy.typewriter</td><td>Armenian Typewriter</td></tr><tr><td>hy.west</td><td>Armenian Western (Legacy)</td></tr><tr><td>ig-Latn</td><td>Igbo</td></tr><tr><td>is-IS</td><td>Icelandic</td></tr><tr><td>it-IT</td><td>Italian</td></tr><tr><td>it-IT.142</td><td>Italian (142)</td></tr><tr><td>iu-Cans</td><td>Inuktitut - Naqittaut</td></tr><tr><td>iu-Cans-CA</td><td>Inuktitut - Nattilik</td></tr><tr><td>iu-La</td><td>Inuktitut - Latin</td></tr><tr><td>ja</td><td>Japanese</td></tr><tr><td>jv-Java</td><td>Javanese</td></tr><tr><td>ka.ergo</td><td>Georgian (Ergonomic)</td></tr><tr><td>ka.legacy</td><td>Georgian (Legacy)</td></tr><tr><td>ka.mes</td><td>Georgian (MES)</td></tr><tr><td>ka.oldalpha</td><td>Georgian (Old Alphabets)</td></tr><tr><td>ka.qwerty</td><td>Georgian (QWERTY)</td></tr><tr><td>khb-Talu</td><td>New Tai Lue</td></tr><tr><td>kk-KZ</td><td>Kazakh</td></tr><tr><td>kl</td><td>Greenlandic</td></tr><tr><td>km</td><td>Khmer</td></tr><tr><td>km.nida</td><td>Khmer (NIDA)</td></tr><tr><td>kn</td><td>Kannada</td></tr><tr><td>ko</td><td>Korean</td></tr><tr><td>ku-Arab</td><td>Central Kurdish</td></tr><tr><td>ky-KG</td><td>Kyrgyz Cyrillic</td></tr><tr><td>lb-LU</td><td>Luxembourgish</td></tr><tr><td>lis-Lisu</td><td>Lisu (Standard)</td></tr><tr><td>lis-Lisu.basic</td><td>Lisu (Basic)</td></tr><tr><td>lo</td><td>Lao</td></tr><tr><td>lt</td><td>Lithuanian Standard</td></tr><tr><td>lt-LT</td><td>Lithuanian</td></tr><tr><td>lt-LT.ibm</td><td>Lithuanian IBM</td></tr><tr><td>lv</td><td>Latvian (Standard)</td></tr><tr><td>lv-LV</td><td>Latvian</td></tr><tr><td>lv-LV.qwerty</td><td>Latvian (QWERTY)</td></tr><tr><td>mi-NZ</td><td>Maori</td></tr><tr><td>mk</td><td>Macedonian - Standard</td></tr><tr><td>mk-MK</td><td>Macedonian</td></tr><tr><td>ml</td><td>Malayalam</td></tr><tr><td>mn-MN</td><td>Mongolian Cyrillic</td></tr><tr><td>mn-Mong</td><td>Traditional Mongolian (Standard)</td></tr><tr><td>mn-Mong-CN</td><td>Traditional Mongolian (MNS)</td></tr><tr><td>mn-Mong.script</td><td>Mongolian (Mongolian Script)</td></tr><tr><td>mn-Phag</td><td>Phags-pa</td></tr><tr><td>mr</td><td>Marathi</td></tr><tr><td>mt-MT.47</td><td>Maltese 47-Key</td></tr><tr><td>mt-MT.48</td><td>Maltese 48-Key</td></tr><tr><td>my.phonetic</td><td>Myanmar (Phonetic order)</td></tr><tr><td>my.visual</td><td>Myanmar (Visual order)</td></tr><tr><td>nb-NO</td><td>Norwegian</td></tr><tr><td>ne-NP</td><td>Nepali</td></tr><tr><td>nl-BE</td><td>Belgian (Period)</td></tr><tr><td>nl-NL</td><td>Dutch</td></tr><tr><td>nqo</td><td>N’Ko</td></tr><tr><td>nso</td><td>Sesotho sa Leboa</td></tr><tr><td>or</td><td>Odia</td></tr><tr><td>osa-Osge</td><td>Osage</td></tr><tr><td>pa</td><td>Punjabi</td></tr><tr><td>pl-PL</td><td>Polish (214)</td></tr><tr><td>pl-PL.programmers</td><td>Polish (Programmers)</td></tr><tr><td>ps</td><td>Pashto (Afghanistan)</td></tr><tr><td>pt-BR.abnt</td><td>Portuguese (Brazil ABNT)</td></tr><tr><td>pt-BR.abnt2</td><td>Portuguese (Brazil ABNT2)</td></tr><tr><td>pt-PT</td><td>Portuguese</td></tr><tr><td>ro-RO</td><td>Romanian (Standard)</td></tr><tr><td>ro-RO.legacy</td><td>Romanian (Legacy)</td></tr><tr><td>ro-RO.programmers</td><td>Romanian (Programmers)</td></tr><tr><td>ru</td><td>Russian - Mnemonic</td></tr><tr><td>ru-RU</td><td>Russian</td></tr><tr><td>ru-RU.typewriter</td><td>Russian (Typewriter)</td></tr><tr><td>sah-Cyrl</td><td>Sakha</td></tr><tr><td>sat-Olck</td><td>Ol Chiki</td></tr><tr><td>se-NO</td><td>Norwegian with Sami</td></tr><tr><td>se-NO.ext_norway</td><td>Sami Extended Norway</td></tr><tr><td>se-SE</td><td>Swedish with Sami</td></tr><tr><td>se-SE.ext_finland_sweden</td><td>Sami Extended Finland-Sweden</td></tr><tr><td>sga-Ogam</td><td>Ogham</td></tr><tr><td>si</td><td>Sinhala</td></tr><tr><td>si.wij9</td><td>Sinhala - Wij 9</td></tr><tr><td>sk-SK</td><td>Slovak</td></tr><tr><td>sk-SK.qwerty</td><td>Slovak (QWERTY)</td></tr><tr><td>sl-SI</td><td>Slovenian</td></tr><tr><td>so-Osma</td><td>Osmanya</td></tr><tr><td>sq</td><td>Albanian</td></tr><tr><td>sr-Cy</td><td>Serbian (Cyrillic)</td></tr><tr><td>sr-La</td><td>Serbian (Latin)</td></tr><tr><td>srb-Sora</td><td>Sora</td></tr><tr><td>sv-SE</td><td>Swedish</td></tr><tr><td>syr-Syrc</td><td>Syriac</td></tr><tr><td>syr-Syrc.phonetic</td><td>Syriac Phonetic</td></tr><tr><td>ta-IN</td><td>Tamil</td></tr><tr><td>ta-IN.99</td><td>Tamil 99</td></tr><tr><td>ta-IN.anjal</td><td>Tamil Anjal</td></tr><tr><td>tdd-Tale</td><td>Tai Le</td></tr><tr><td>te</td><td>Telugu</td></tr><tr><td>tg-Cyrl</td><td>Tajik</td></tr><tr><td>th</td><td>Thai Kedmanee</td></tr><tr><td>th.non-shiftlock</td><td>Thai Kedmanee (non-ShiftLock)</td></tr><tr><td>th.pattachote</td><td>Thai Pattachote</td></tr><tr><td>th.pattachote.non-shiftlock</td><td>Thai Pattachote (non-ShiftLock)</td></tr><tr><td>tk-Latn</td><td>Turkmen</td></tr><tr><td>tn-ZA</td><td>Setswana</td></tr><tr><td>tr-TR.f</td><td>Turkish F</td></tr><tr><td>tr-TR.q</td><td>Turkish Q</td></tr><tr><td>tt-Cyrl</td><td>Tatar</td></tr><tr><td>tt-RU</td><td>Tatar (Legacy)</td></tr><tr><td>tzm-Latn</td><td>Central Atlas Tamazight</td></tr><tr><td>tzm-Tfng</td><td>Tifinagh (Basic)</td></tr><tr><td>tzm-Tfng.ex</td><td>Tifinagh (Extended)</td></tr><tr><td>ug-Arab</td><td>Uyghur</td></tr><tr><td>ug-Arab.legacy</td><td>Uyghur (Legacy)</td></tr><tr><td>uk</td><td>Ukrainian (Enhanced)</td></tr><tr><td>uk-UA</td><td>Ukrainian</td></tr><tr><td>ur-PK</td><td>Urdu</td></tr><tr><td>uz-Cy</td><td>Uzbek Cyrillic</td></tr><tr><td>vi</td><td>Vietnamese</td></tr><tr><td>wo-Latn</td><td>Wolof</td></tr><tr><td>yo-Latn</td><td>Yoruba</td></tr><tr><td>zh-Hans-CN</td><td>Chinese (Simplified) - US</td></tr><tr><td>zh-Hans-SG</td><td>Chinese (Simplified, Singapore) - US</td></tr><tr><td>zh-Hant-HK</td><td>Chinese (Traditional, Hong Kong S.A.R.) - US</td></tr><tr><td>zh-Hant-MO</td><td>Chinese (Traditional, Macao S.A.R.) - US</td></tr><tr><td>zh-Hant-TW</td><td>Chinese (Traditional) - US</td></tr></table></details>
#_advanced
#keyboard_layout_proposals = en-US, fr-FR, de-DE

# Display the close screen.
# Displays secondary connection errors and closes automatically after the specified [internal_mod]close_box_timeout value or on user request.
# (type: boolean (0/no/false or 1/yes/true))
#enable_close_box = 1

# Specifies the time to spend on the close box of the RDP proxy before closing client window.
# ⚠ Value 0 deactivates the timer and the connection remains open until the client disconnects.
# (in seconds)
#_advanced
#close_box_timeout = 600

[mod_replay]

# (maxlen = 4096)
# (acl config: proxy ⇐ replay_path)
#replay_path = /tmp/

# 0 - Wait for Escape, 1 - End session
# (type: boolean (0/no/false or 1/yes/true))
#on_end_of_data = 0

# 0 - replay once, 1 - loop replay
# (type: boolean (0/no/false or 1/yes/true))
# (acl config: proxy ⇐ replay_on_loop)
#replay_on_loop = 0

[translation]

# The login page shows this language to all users. Once logged in, users see their preferred language.
#   Auto: The language is determined based on the keyboard layout specified by the client.
#   EN: 
#   FR: 
#_advanced
#login_language = Auto

[theme]

# Enable custom theme color configuration.
# (type: boolean (0/no/false or 1/yes/true))
#enable_theme = 0

# Logo displayed when theme is enabled.
#logo = )gen_config_ini" << (REDEMPTION_CONFIG_THEME_LOGO) << R"gen_config_ini(

# Background color for window, label and button.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#bgcolor = #081F60

# Foreground color for window, label and button.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#fgcolor = #FFFFFF

# Separator line color used with some widgets.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#separator_color = #CFD5EB

# Background color used by buttons when they have focus.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#focus_color = #004D9C

# Text color for error messages. For example, an authentication error in the login.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#error_color = #FFFF00

# Background color for editing field.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#edit_bgcolor = #FFFFFF

# Foreground color for editing field.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#edit_fgcolor = #000000

# Outline color for editing field.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#edit_border_color = #081F60

# Outline color for editing field that has focus.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#edit_focus_border_color = #004D9C

# Cursor color for editing field.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#edit_cursor_color = #888888

# Placeholder text color for editing field with a little resolution.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#edit_placeholder_color = #A0A0A0

# Foreground color for toggle button of password field.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#password_toggle_color = #A0A0A0

# Background color for tooltip.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#tooltip_bgcolor = #FFFF9F

# Foreground color for tooltip.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#tooltip_fgcolor = #000000

# Border color for tooltip.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#tooltip_border_color = #000000

# Background color for even rows in the selector widget.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_line1_bgcolor = #E9ECF6

# Foreground color for even rows in the selector widget.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_line1_fgcolor = #000000

# Background color for odd rows in the selector widget.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_line2_bgcolor = #CFD5EB

# Foreground color for odd rows in the selector widget.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_line2_fgcolor = #000000

# Background color for the row that has focus in the selector widget.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_focus_bgcolor = #004D9C

# Foreground color for the row that has focus in the selector widget.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_focus_fgcolor = #FFFFFF

# Background color for the row that is selected in the selector widget but does not have focus.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_selected_bgcolor = #4472C4

# Foreground color for the row that is selected in the selector widget but does not have focus.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_selected_fgcolor = #FFFFFF

# Background color for name of filter fields in the selector widget.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_label_bgcolor = #4472C4

# Foreground color for name of filter fields in the selector widget.
# (in rgb format: hexadecimal (0x21AF21), #rgb (#2fa), #rrggbb (#22ffaa) or a named color case insensitive ("https://en.wikipedia.org/wiki/Web_colors#Extended_colors"))
#selector_label_fgcolor = #FFFFFF

[debug]

# Restrict target debugging to a specific primary user.
#_advanced
#primary_user = 

#_display_name=Fake target IP
#fake_target_ip = 

# - kbd / ocr when != 0
# 
# (Wrm)
# - pointer             = 0x0004
# - primary_orders      = 0x0020
# - secondary_orders    = 0x0040
# - bitmap_update       = 0x0080
# - surface_commands    = 0x0100
# - bmp_cache           = 0x0200
# - internal_buffer     = 0x0400
# - sec_decrypted       = 0x1000
# (min = 0)
#_advanced
#capture = 0

# - variable = 0x0002
# - buffer   = 0x0040
# - dump     = 0x1000
# (min = 0)
#_advanced
#auth = 0

# - Log   = 0x01
# - Event = 0x02
# - Acl   = 0x04
# - Trace = 0x08
# (min = 0)
#_advanced
#session = 0

# - basic_trace     = 0x00000001
# - basic_trace2    = 0x00000002
# - basic_trace3    = 0x00000004
# - basic_trace4    = 0x00000008
# - basic_trace5    = 0x00000020
# - graphic         = 0x00000040
# - channel         = 0x00000080
# - cache_from_disk = 0x00000400
# - bmp_info        = 0x00000800
# - global_channel  = 0x00002000
# - sec_decrypted   = 0x00004000
# - keymap          = 0x00008000
# - nla             = 0x00010000
# - nla_dump        = 0x00020000
# 
# (Serializer)
# - pointer             = 0x00040000
# - primary_orders      = 0x00200000
# - secondary_orders    = 0x00400000
# - bitmap_update       = 0x00800000
# - surface_commands    = 0x01000000
# - bmp_cache           = 0x02000000
# - internal_buffer     = 0x04000000
# - sec_decrypted       = 0x10000000
# (min = 0)
#_advanced
#front = 0

# - basic_trace         = 0x00000001
# - connection          = 0x00000002
# - security            = 0x00000004
# - capabilities        = 0x00000008
# - license             = 0x00000010
# - asynchronous_task   = 0x00000020
# - graphics_pointer    = 0x00000040
# - graphics            = 0x00000080
# - input               = 0x00000100
# - rail_order          = 0x00000200
# - credssp             = 0x00000400
# - negotiation         = 0x00000800
# - cache_persister     = 0x00001000
# - fsdrvmgr            = 0x00002000
# - sesprobe_launcher   = 0x00004000
# - sesprobe_repetitive = 0x00008000
# - drdynvc             = 0x00010000
# - surfaceCmd          = 0x00020000
# - cache_from_disk     = 0x00040000
# - bmp_info            = 0x00080000
# - drdynvc_dump        = 0x00100000
# - printer             = 0x00200000
# - rdpsnd              = 0x00400000
# - channels            = 0x00800000
# - rail                = 0x01000000
# - sesprobe            = 0x02000000
# - cliprdr             = 0x04000000
# - rdpdr               = 0x08000000
# - rail_dump           = 0x10000000
# - sesprobe_dump       = 0x20000000
# - cliprdr_dump        = 0x40000000
# - rdpdr_dump          = 0x80000000
# (min = 0)
#_advanced
#_display_name=Mod RDP
#mod_rdp = 0

# - basic_trace     = 0x00000001
# - keymap_stack    = 0x00000002
# - draw_event      = 0x00000004
# - input           = 0x00000008
# - connection      = 0x00000010
# - hextile_encoder = 0x00000020
# - cursor_encoder  = 0x00000040
# - clipboard       = 0x00000080
# - zrle_encoder    = 0x00000100
# - zrle_trace      = 0x00000200
# - hextile_trace   = 0x00000400
# - cursor_trace    = 0x00001000
# - rre_encoder     = 0x00002000
# - rre_trace       = 0x00004000
# - raw_encoder     = 0x00008000
# - raw_trace       = 0x00010000
# - copyrect_encoder= 0x00020000
# - copyrect_trace  = 0x00040000
# - keymap          = 0x00080000
# (min = 0)
#_advanced
#_display_name=Mod VNC
#mod_vnc = 0

# - copy_paste != 0
# - client_execute = 0x01
# (min = 0)
#_advanced
#mod_internal = 0

# - basic    = 0x0001
# - dump     = 0x0002
# - watchdog = 0x0004
# - meta     = 0x0008
# (min = 0)
#_advanced
#sck_mod = 0

# - basic    = 0x0001
# - dump     = 0x0002
# - watchdog = 0x0004
# - meta     = 0x0008
# (min = 0)
#_advanced
#sck_front = 0

# (min = 0)
#password = 0

# - when != 0
# (min = 0)
#_advanced
#compression = 0

# - life       = 0x0001
# - persistent = 0x0200
# (min = 0)
#_advanced
#cache = 0

# - when != 0
# (min = 0)
#_advanced
#ocr = 0

# Value passed to function av_log_set_level()
# See https://www.ffmpeg.org/doxygen/2.3/group__lavu__log__constants.html
# (min = 0)
#_advanced
#_display_name=FFmpeg
#ffmpeg = 0

# Log unknown members or sections.
# (type: boolean (0/no/false or 1/yes/true))
#_advanced
#config = 1

#   0: Off
#   1: SimulateErrorRead
#   2: SimulateErrorWrite
#_display_name=Mod RDP use failure simulation socket transport
#mod_rdp_use_failure_simulation_socket_transport = 0

# List of client probe IP addresses (e.g. ip1, ip2, etc.) to prevent some continuous logs.
# (values are comma-separated)
#_advanced
#probe_client_addresses = 

)gen_config_ini"
