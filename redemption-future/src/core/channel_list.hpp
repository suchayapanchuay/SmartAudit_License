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
  Copyright (C) Wallix 2010
  Author(s): Christophe Grosjean, Javier Caverni
  Based on xrdp Copyright (C) Jay Sorg 2004-2010

  Channels descriptors
*/


#pragma once

#include "core/channel_names.hpp"
#include "utils/sugar/array_view.hpp"
#include "utils/log.hpp"

#include <cassert>


namespace CHANNELS {
    enum {
        MAX_STATIC_VIRTUAL_CHANNELS = 30 // 30 static virtual channels
    };

    //    1.3.3 Static Virtual Channels
    //    =============================

    //    Static virtual channels allow lossless communication between client and server components over the
    //    main RDP data connection. Virtual channel data is application-specific and opaque to RDP. A
    //    maximum of 30 static virtual channels can be created at connection time.
    //    The list of desired virtual channels is requested and confirmed during the Basic Settings Exchange
    //    phase of the connection sequence (as specified in section 1.3.1.1) and the endpoints are joined
    //    during the Channel Connection phase (as specified in section 1.3.1.1). Once joined, the client and
    //    server endpoints should be prevented from exchanging data until the connection sequence has
    //    completed.
    //    Static virtual channel data must be broken up into chunks of up to 1600 bytes in size before being
    //    transmitted (this size does not include "DP headers). Each virtual channel acts as an independent
    //    data stream. The client and server examine the data received on each virtual channel and route the
    //    data stream to the appropriate endpoint for further processing. A particular client or server
    //    implementation can decide whether to pass on individual chunks of data as they are received, or to
    //    assemble the separate chunks of data into a complete block before passing it on to the endpoint.

    struct ChannelDef
    {
        ChannelNameId name;
        uint32_t flags{0};
        uint16_t chanid{0};

        ChannelDef() = default;

        ChannelDef(ChannelNameId name, uint32_t flags, uint16_t chanid)
        : name(name)
        , flags(flags)
        , chanid(chanid)
        {}

        void log(uint16_t index) const
        {
            LOG(LOG_INFO, "ChannelDef[%u]::(name = %s, flags = 0x%08X, chanid = %u)",
                index, this->name, this->flags, this->chanid);
        }
    };

    class ChannelDefArrayView
    {
        array_view<ChannelDef> channels;

    public:
        ChannelDefArrayView() = default;

        ChannelDefArrayView(array_view<ChannelDef> channels)
        : channels(channels)
        {}

        const ChannelDef & operator[](size_t index) const
        {
            return this->channels[index];
        }

        size_t size() const
        {
            return this->channels.size();
        }

        const ChannelDef * begin() const
        {
            return this->channels.begin();
        }

        const ChannelDef * end() const
        {
            return this->channels.begin();
        }

        const ChannelDef * get_by_name(ChannelNameId name) const
        {
            for (ChannelDef const& chann : this->channels) {
                if (name == chann.name) {
                    return &chann;
                }
            }
            return nullptr;
        }

        const ChannelDef * get_by_id(uint16_t chanid) const
        {
            for (ChannelDef const& chann : this->channels) {
                if (chanid == chann.chanid) {
                    return &chann;
                }
            }
            return nullptr;
        }

        void log(char const * name) const
        {
            LOG(LOG_INFO, "%s channels %zu channels defined", name, this->channels.size());
            uint16_t index = 0;
            for (auto const& channel : this->channels) {
                channel.log(index++);
            }
        }
    };

    class ChannelDefArray
    {
        static constexpr size_t max_len_channel = MAX_STATIC_VIRTUAL_CHANNELS + 2; // + global channel + wab channel

        // The number of requested static virtual channels (the maximum allowed is 31).
        // TODO static_vector<., 32>
        size_t     channelCount{0};
        ChannelDef items[max_len_channel];

    public:
        ChannelDefArray() = default;

        void clear_channels()
        {
            this->channelCount = 0;
        }

        const ChannelDef & operator[](size_t index) const
        {
            return this->items[index];
        }

        void set_chanid(size_t index, uint16_t chanid)
        {
            this->items[index].chanid = chanid;
        }

        [[nodiscard]]
        size_t size() const
        {
            return this->channelCount;
        }

        const ChannelDef * begin() const
        {
            return this->items;
        }

        const ChannelDef * end() const
        {
            return this->begin() + size();
        }

        void push_back(const ChannelDef & item)
        {
            assert(this->channelCount < max_len_channel);
            this->items[this->channelCount] = item;
            this->channelCount++;
        }

        ChannelDefArrayView view() const&& = delete;

        ChannelDefArrayView view() const&
        {
            return ChannelDefArrayView(array_view{items, channelCount});
        }

        operator ChannelDefArrayView() const&& = delete;

        operator ChannelDefArrayView() const&
        {
            return view();
        }

        [[nodiscard]]
        const ChannelDef * get_by_name(ChannelNameId name) const
        {
            return view().get_by_name(name);
        }

        [[nodiscard]]
        const ChannelDef * get_by_id(uint16_t chanid) const
        {
            return view().get_by_id(chanid);
        }

        void log(char const * name) const
        {
            view().log(name);
        }
    };

    // [MS-RDPBCGR] 2.2.6.1 Virtual Channel PDU
    // ========================================

    // The Virtual Channel PDU is sent from client to server or from server to
    //  client and is used to transport data between static virtual channel
    //  endpoints.

    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
    // |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |                           tpktHeader                          |
    // +-----------------------------------------------+---------------+
    // |                    x224Data                   |     mcsPdu    |
    // |                                               |   (variable)  |
    // +-----------------------------------------------+---------------+
    // |                              ...                              |
    // +---------------------------------------------------------------+
    // |                   securityHeader (variable)                   |
    // +---------------------------------------------------------------+
    // |                              ...                              |
    // +---------------------------------------------------------------+
    // |                        channelPduHeader                       |
    // +---------------------------------------------------------------+
    // |                              ...                              |
    // +---------------------------------------------------------------+
    // |                 virtualChannelData (variable)                 |
    // +---------------------------------------------------------------+
    // |                              ...                              |
    // +---------------------------------------------------------------+

    // tpktHeader (4 bytes): A TPKT Header, as specified in [T123] section 8.

    // x224Data (3 bytes): An X.224 Class 0 Data TPDU, as specified in [X224]
    //  section 13.7.

    // mcsPdu (variable): If the PDU is being sent from client to server, this
    //  field MUST contain a variable-length, PER-encoded MCS Domain PDU
    //  (DomainMCSPDU) which encapsulates an MCS Send Data Request structure
    //  (SDrq, choice 25 from DomainMCSPDU), as specified in [T125] section 11.32
    //  (the ASN.1 structure definition is given in [T125] section 7, parts 7 and
    //  10). The userData field of the MCS Send Data Request contains a Security
    //  Header and the static virtual channel data.
    //
    // If the PDU is being sent from server to client, this field MUST contain a
    //  variable-length, PER-encoded MCS Domain PDU (DomainMCSPDU) which
    //  encapsulates an MCS Send Data Indication structure (SDin, choice 26 from
    //  DomainMCSPDU), as specified in [T125] section 11.33 (the ASN.1 structure
    //  definition is given in [T125] section 7, parts 7 and 10). The userData
    //  field of the MCS Send Data Indication contains a Security Header and the
    //  static virtual channel data.

    // securityHeader (variable): Optional security header. The presence and
    //  format of the security header depends on the Encryption Level and
    //  Encryption Method selected by the server (sections 5.3.2 and 2.2.1.4.3).
    //  If the Encryption Level selected by the server is greater than
    //  ENCRYPTION_LEVEL_NONE (0) and the Encryption Method selected by the server
    //  is greater than ENCRYPTION_METHOD_NONE (0), then this field MUST contain
    //  one of the security headers described in section 2.2.8.1.1.2.
    //
    // If the PDU is being sent from client to server:
    //
    // * The securityHeader field MUST contain a Non-FIPS Security Header (section
    //   2.2.8.1.1.2.2) if the Encryption Method selected by the server is
    //   ENCRYPTION_METHOD_40BIT (0x00000001), ENCRYPTION_METHOD_56BIT
    //   (0x00000008), or ENCRYPTION_METHOD_128BIT (0x00000002).
    //
    // If the PDU is being sent from server to client:
    //
    // * The securityHeader field MUST contain a Basic Security Header (section
    //   2.2.8.1.1.2.1) if the Encryption Level selected by the server is
    //   ENCRYPTION_LEVEL_LOW (1).
    //
    // * The securityHeader field MUST contain a Non-FIPS Security Header (section
    //   2.2.8.1.1.2.2) if the Encryption Method selected by the server is
    //   ENCRYPTION_METHOD_40BIT (0x00000001), ENCRYPTION_METHOD_56BIT
    //   (0x00000008), or ENCRYPTION_METHOD_128BIT (0x00000002).
    //
    // If the Encryption Method selected by the server is ENCRYPTION_METHOD_FIPS
    //  (0x00000010) the securityHeader field MUST contain a FIPS Security Header
    //  (section 2.2.8.1.1.2.3).
    //
    // If the Encryption Level selected by the server is ENCRYPTION_LEVEL_NONE (0)
    //  and the Encryption Method selected by the server is ENCRYPTION_METHOD_NONE
    //  (0), then this header MUST NOT be included in the PDU.

    // channelPduHeader (8 bytes): A Channel PDU Header (section 2.2.6.1.1)
    //  structure, which contains control flags and describes the size of the
    //  opaque channel data.

    // virtualChannelData (variable): Variable-length data to be processed by the
    //  static virtual channel protocol handler. This field MUST NOT be larger
    //  than CHANNEL_CHUNK_LENGTH (1600) bytes in size unless the maximum virtual
    //  channel chunk size is specified in the optional VCChunkSize field of the
    //  Virtual Channel Capability Set (section 2.2.7.1.10).

    enum {
          CHANNEL_CHUNK_LENGTH = 1600,

          MAX_CHANNEL_CHUNK_LENGTH = 16256
    };

    // [MS-RDPBCGR] 2.2.6.1.1 Channel PDU Header (CHANNEL_PDU_HEADER)
    // ==============================================================

    // The CHANNEL_PDU_HEADER MUST precede all opaque static virtual channel
    //  traffic chunks transmitted via RDP between a client and server.

    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
    // |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |                             length                            |
    // +---------------------------------------------------------------+
    // |                             flags                             |
    // +---------------------------------------------------------------+

    // length (4 bytes): A 32-bit, unsigned integer. The total length in bytes of
    //  the uncompressed channel data, excluding this header. The data can span
    //  multiple Virtual Channel PDUs and the individual chunks will need to be
    //  reassembled in that case (section 3.1.5.2.2).

    // flags (4 bytes): A 32-bit, unsigned integer. The channel control flags.

    // +----------------------------+----------------------------------------------+
    // | Flag                       | Meaning                                      |
    // +----------------------------+----------------------------------------------+
    // | CHANNEL_FLAG_FIRST         | Indicates that the chunk is the first in a   |
    // | 0x00000001                 | sequence.                                    |
    // +----------------------------+----------------------------------------------+
    // | CHANNEL_FLAG_LAST          | Indicates that the chunk is the last in a    |
    // | 0x00000002                 | sequence.                                    |
    // +----------------------------+----------------------------------------------+
    // | CHANNEL_FLAG_SHOW_PROTOCOL | The Channel PDU Header MUST be visible to    |
    // | 0x00000010                 | the application endpoint (see section        |
    // |                            | 2.2.1.3.4.1).                                |
    // +----------------------------+----------------------------------------------+
    // | CHANNEL_FLAG_SUSPEND       | All virtual channel traffic MUST be          |
    // | 0x00000020                 | suspended. This flag is only valid in        |
    // |                            | server-to-client virtual channel traffic. It |
    // |                            | MUST be ignored in client-to-server data.    |
    // +----------------------------+----------------------------------------------+
    // | CHANNEL_FLAG_RESUME        | All virtual channel traffic MUST be resumed. |
    // | 0x00000040                 | This flag is only valid in server-to-client  |
    // |                            | virtual channel traffic. It MUST be ignored  |
    // |                            | in client-to-server data.                    |
    // +----------------------------+----------------------------------------------+
    // | CHANNEL_PACKET_COMPRESSED  | The virtual channel data is compressed. This |
    // | 0x00200000                 | flag is equivalent to MPPC bit C (for more   |
    // |                            | information see [RFC2118] section 3.1).      |
    // +----------------------------+----------------------------------------------+
    // | CHANNEL_PACKET_AT_FRONT    | The decompressed packet MUST be placed at    |
    // | 0x00400000                 | the beginning of the history buffer. This    |
    // |                            | flag is equivalent to MPPC bit B (for more   |
    // |                            | information see [RFC2118] section 3.1).      |
    // +----------------------------+----------------------------------------------+
    // | CHANNEL_PACKET_FLUSHED     | The decompressor MUST reinitialize the       |
    // | 0x00800000                 | history buffer (by filling it with zeros)    |
    // |                            | and reset the HistoryOffset to zero. After   |
    // |                            | it has been reinitialized, the entire        |
    // |                            | history buffer is immediately regarded as    |
    // |                            | valid. This flag is equivalent to MPPC bit A |
    // |                            | (for more information see [RFC2118] section  |
    // |                            | 3.1). If the CHANNEL_PACKET_COMPRESSED       |
    // |                            | (0x00200000) flag is also present, then the  |
    // |                            | CHANNEL_PACKET_FLUSHED flag MUST be          |
    // |                            | processed first.                             |
    // +----------------------------+----------------------------------------------+
    // | CompressionTypeMask        | Indicates the compression package which was  |
    // | 0x000F0000                 | used to compress the data. See the           |
    // |                            | discussion which follows this table for a    |
    // |                            | list of compression packages.                |
    // +----------------------------+----------------------------------------------+

    enum {
          CHANNEL_FLAG_FIRST         = 0x00000001
        , CHANNEL_FLAG_LAST          = 0x00000002
        , CHANNEL_FLAG_SHOW_PROTOCOL = 0x00000010
        , CHANNEL_FLAG_SUSPEND       = 0x00000020
        , CHANNEL_FLAG_RESUME        = 0x00000040
        , CHANNEL_PACKET_COMPRESSED  = 0x00200000
        , CHANNEL_PACKET_AT_FRONT    = 0x00400000
        , CHANNEL_PACKET_FLUSHED     = 0x00800000
        , CompressionTypeMask        = 0x000F0000
    };
}   // namespace CHANNELS
