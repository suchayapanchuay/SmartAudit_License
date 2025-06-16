/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2012
   Author(s): Christophe Grosjean

   RDP Capabilities : Bitmap Codecs Capability Set ([MS-RDPBCGR] section 2.2.7.2.10)

*/

#pragma once

#include "utils/log.hpp"
#include "core/error.hpp"
#include "utils/stream.hpp"
#include "utils/hexdump.hpp"
#include "core/RDP/capabilities/common.hpp"
#include "core/stream_throw_helpers.hpp"

#include <array>
#include <vector>
#include <variant>
#include <memory>
#include <cstring>


// 2.2.7.2.10 Bitmap Codecs Capability Set (TS_BITMAPCODECS_CAPABILITYSET)
// =======================================================================
// The TS_BITMAPCODECS_CAPABILITYSET structure advertises support for bitmap encoding and
// decoding codecs used in conjunction with the Set Surface Bits Surface Command (section 2.2.9.2.1)
// and Cache Bitmap (Revision 3) Secondary Drawing Order ([MS-RDPEGDI] section 2.2.2.2.1.2.8).
// This capability is sent by both the client and server.

// capabilitySetType (2 bytes): A 16-bit, unsigned integer. The type of capability set. This field
//    MUST be set to 0x001D (CAPSETTYPE_BITMAP_CODECS).

// lengthCapability (2 bytes): A 16-bit, unsigned integer. The length in bytes of the capability
//    data.

// supportedBitmapCodecs (variable): A variable-length field containing a TS_BITMAPCODECS
//    structure (section 2.2.7.2.10.1).

// 2.2.7.2.10.1 Bitmap Codecs (TS_BITMAPCODECS)
// ============================================
// The TS_BITMAPCODECS structure contains an array of bitmap codec capabilities.

// bitmapCodecCount (1 byte): An 8-bit, unsigned integer. The number of bitmap codec
//   capability entries contained in the bitmapCodecArray field (the maximum allowed is 255).
//
// bitmapCodecArray (variable): A variable-length array containing a series of
//   TS_BITMAPCODEC structures (section 2.2.7.2.10.1.1) that describes the supported bitmap
//    codecs. The number of TS_BITMAPCODEC structures contained in the array is given by the
//    bitmapCodecCount field.

// 2.2.7.2.10.1.1 Bitmap Codec (TS_BITMAPCODEC)
// ============================================
// The TS_BITMAPCODEC structure is used to describe the encoding parameters of a bitmap codec.

// codecGUID (16 bytes): A Globally Unique Identifier (section 2.2.7.2.10.1.1.1) that functions
//    as a unique ID for each bitmap codec.
//    +------------------------------------+----------------------------------------------------+
//    | Value                              | Meaning                                            |
//    +------------------------------------+----------------------------------------------------+
//    | CODEC_GUID_NSCODEC                 | The Bitmap Codec structure defines encoding        |
//    | 0xCA8D1BB9000F154F589FAE2D1A87E2D6 | parameters for the NSCodec Bitmap Codec ([MS-      |
//    |                                    | RDPNSC] sections 2 and 3). The codecProperties     |
//    |                                    | field MUST contain an NSCodec Capability Set ([MS- |
//    |                                    | RDPNSC] section 2.2.1) structure.                  |
//    +------------------------------------+----------------------------------------------------+
//    | CODEC_GUID_REMOTEFX                | The Bitmap Codec structure defines encoding        |
//    | 0x76772F12BD724463AFB3B73C9C6F7886 | parameters for the RemoteFX Bitmap Codec ([MS-     |
//    |                                    | RDPRFX] sections 2 and 3). The codecProperties     |
//    |                                    | field MUST contain a                               |
//    |                                    | TS_RFX_CLNT_CAPS_CONTAINER ([MS-RDPRFX]            |
//    |                                    | section 2.2.1.1) structure or a                    |
//    |                                    | TS_RFX_SRVR_CAPS_CONTAINER ([MS-RDPRFX]            |
//    |                                    | section 2.2.1.2) structure.                        |
//    +------------------------------------+----------------------------------------------------+

// codecID (1 byte): An 8-bit unsigned integer. When sent from the client to the server, this field
//    contains a unique 8-bit ID that can be used to identify bitmap data encoded using the codec in
//    wire traffic associated with the current connection - this ID is used in subsequent Set Surface
//    Bits commands (section 2.2.9.2.1) and Cache Bitmap (Revision 3) orders ([MS-RDPEGDI]
//    section 2.2.2.2.1.2.8). When sent from the server to the client, the value in this field is
//    ignored by the client - the client determines the 8-bit ID to use for the codec. If the
//    codecGUID field contains the CODEC_GUID_NSCODEC GUID, then this field MUST be set to
//    0x01 (the codec ID 0x01 MUST NOT be associated with any other bitmap codec).

// codecPropertiesLength (2 bytes): A 16-bit, unsigned integer. The size in bytes of the
//    codecProperties field.

// codecProperties (variable): A variable-length array of bytes containing data that describes
//    the encoding parameter of the bitmap codec. If the codecGUID field is set to
//    CODEC_GUID_NSCODEC, this field MUST contain an NSCodec Capability Set ([MS-RDPNSC]
//    section 2.2.1) structure. Otherwise, if the codecGUID field is set to
//    CODEC_GUID_REMOTEFX, this field MUST contain a TS_RFX_CLNT_CAPS_CONTAINER ([MS-
//    RDPRFX] section 2.2.1.1) structure when sent from client to server, and a
//    TS_RFX_SRVR_CAPS_CONTAINER ([MS-RDPRFX] section 2.2.1.2) structure when sent from
//    server to client.



// 2.2.1.1 TS_RFX_CLNT_CAPS_CONTAINER
// ==================================
// The TS_RFX_CLNT_CAPS_CONTAINER structure is the top-level client capability container that
// wraps a TS_RFX_CAPS (section 2.2.1.1.1) structure and is sent from the client to the server. It is
// encapsulated in the codecProperties field of the Bitmap Codec ([MS-RDPBCGR] section
// 2.2.7.2.10.1.1) structure, which is ultimately encapsulated in the Bitmap Codecs Capability Set
// ([MS-RDPBCGR] section 2.2.7.2.10), which is encapsulated in a client-to-server Confirm Active PDU
// ([MS-RDPBCGR] section 2.2.1.13.2).

// length (4 bytes): A 32-bit, unsigned integer. Specifies the combined size, in bytes, of the
//    length, captureFlags, capsLength, and capsData fields.

// captureFlags (4 bytes): A 32-bit, unsigned integer. A collection of flags that allow a client to
//    control how data is captured and transmitted by the server.
//    +----------------------------+---------------------------------------------------------------+
//    | Flag                       | Meaning                                                       |
//    +----------------------------+---------------------------------------------------------------+
//    | CARDP_CAPS_CAPTURE_NON_CAC | The client supports mixing RemoteFX data with data            |
//    | 0x00000001                 | compressed by other codecs. The set of other codecs           |
//    |                            | supported by the client will be negotiated using the Bitmap   |
//    |                            | Codecs Capability Set ([MS-RDPBCGR] section 2.2.7.2.10).      |
//    +----------------------------+---------------------------------------------------------------+
// capsLength (4 bytes): A 32-bit, unsigned integer. Specifies the size, in bytes, of the
//    capsData field.
//
// capsData (variable): A variable-sized field that contains a TS_RFX_CAPS (section 2.2.1.1.1)
//    structure.

// 2.2.1.1.1 TS_RFX_CAPS
// =====================
// The TS_RFX_CAPS structure contains information about the decoder capabilities.

// blockType (2 bytes): A 16-bit, unsigned integer. Specifies the data block type. This field MUST
//    be set to CBY_CAPS (0xCBC0).
//
// blockLen (4 bytes): A 32-bit, unsigned integer. Specifies the combined size, in bytes, of the
//    blockType, blockLen, and numCapsets fields. This field MUST be set to 0x0008.
//
// numCapsets (2 bytes): A 16-bit, unsigned integer. Specifies the number of TS_RFX_CAPSET
//    (section 2.2.1.1.1.1) structures contained in the capsetsData field. This field MUST be set to
//    0x0001.
//
// capsetsData (variable): A variable-sized array of TS_RFX_CAPSET (section 2.2.1.1.1.1)
//    structures. The structures in this array MUST be packed on byte boundaries. The blockType
//    and blockLen fields of each TS_RFX_CAPSET structure identify the type and size of the
//    structure.


// 2.2.1.1.1.1.1  TS_RFX_ICAP
// ==========================
// The TS_RFX_ICAP structure specifies the set of codec properties that the decoder supports.

// version (2 bytes): A 16-bit, unsigned integer. Specifies the codec version. This field MUST be
//    set to 0x0100 CLW_VERSION_1_0, to indicate protocol version 1.0.

// tileSize (2 bytes): A 16-bit, signed integer. Specifies the width and height of a tile. This field
//    MUST be set to CT_TILE_64x64 (0x0040), indicating that a tile is 64 x 64 pixels.

// flags (1 byte): An 8-bit, unsigned integer. Specifies operational flags.
//    +------------+---------------------------------------------------------------------------------------+
//    | Flag       | Meaning                                                                               |
//    +------------+---------------------------------------------------------------------------------------+
//    | CODEC_MODE | The codec will operate in image mode. If this flag is not set, the codec will operate |
//    | 0x02       | in video mode.                                                                        |
//    +------------+---------------------------------------------------------------------------------------+
//    When operating in image mode, the encode headers messages (section 2.2.2.2) MUST always
//    precede an encoded frame. When operating in video mode, the header messages MUST be
//    present at the beginning of the stream and are optional elsewhere.

// colConvBits (1 byte): An 8-bit, unsigned integer. Specifies the color conversion transform.
// This field MUST be set to CLW_COL_CONV_ICT (0x1), and the transformation is by the
//  equations in sections 3.1.8.1.3 and 3.1.8.2.5.
//
// transformBits (1 byte): An 8-bit, unsigned integer. Specifies the DWT. This field MUST be set
//    to CLW_XFORM_DWT_53_A (0x1), the DWT transform given by the lifting equations for the
//    DWT shown in section 3.1.8.1.4 and by the lifting equations for the inverse DWT shown in
//    section 3.1.8.2.4.
//
// entropyBits (1 byte): An 8-bit, unsigned integer. Specifies the entropy algorithm. This field
//    MUST be set to one of the following values.
//    +-------------------+-----------------------------------------------------+
//    | Value             | Meaning                                             |
//    +-------------------+-----------------------------------------------------+
//    | CLW_ENTROPY_RLGR1 | RLGR algorithm as described in 3.1.8.1.7.1.         |
//    | 0x01              |                                                     |
//    +-------------------+-----------------------------------------------------+
//    | CLW_ENTROPY_RLGR3 | RLGR algorithm as described in section 3.1.8.1.7.2. |
//    | 0x04              |                                                     |
//    +-------------------+-----------------------------------------------------+

// 2.2.1.2 TS_RFX_SRVR_CAPS_CONTAINER
// ==================================
// The TS_RFX_SRVR_CAPS_CONTAINER structure is the top-level server capability container, which is
// sent from the server to the client. It is encapsulated in the codecProperties field of the Bitmap
// Codec structure ([MS-RDPBCGR] section 2.2.7.2.10.1.1), which is ultimately encapsulated in the
// Bitmap Codecs Capability Set ([MS-RDPBCGR] section 2.2.7.2.10). The Bitmap Codecs Capability
// Set is encapsulated in a server-to-client Demand Active PDU ([MS-RDPBCGR] section 2.2.1.13.1).

// reserved (variable): A variable-sized array of bytes. All the bytes in this field MUST be set to
//    0. The size of the field is given by the corresponding codecPropertiesLength field of the
//    parent TS_BITMAPCODEC, as specified in [MS-RDPBCGR] section 2.2.7.2.10.1.1 Bitmap
//    Codecs Capability Set.


enum {
       CLW_VERSION_1_0 = 0x0100
     };

enum {
       CODEC_MODE = 0x02
     };

enum {
       CLW_COL_CONV_ICT = 0x01
     };

enum {
       CLW_XFORM_DWT_53_A = 0x01
     };

enum {
       CLW_ENTROPY_RLGR1 = 0x01
     , CLW_ENTROPY_RLGR3 = 0x04
     };

enum {
       CBY_CAPS = 0xCBC0
     , CBY_CAPSET = 0xCBC1
     };

enum {
       CLY_CAPSET = 0xCFC0
     };

enum {
       CARDP_CAPS_CAPTURE_NON_CAC = 0x01
     };

enum {
       BITMAPCODECS_MAX_SIZE = 0xFF
     };

enum BitmapCodecType {
    CODEC_IGNORE = 0,
    CODEC_NS,
    CODEC_REMOTEFX,
    CODEC_UNKNOWN,
};

inline const char *bitmapCodecTypeStr(BitmapCodecType btype) {
    REDEMPTION_DIAGNOSTIC_PUSH()
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wcovered-switch-default")
    switch (btype) {
    default:
    case CODEC_UNKNOWN: return "UNKNOWN";
    case CODEC_IGNORE: return "IGNORE";
    case CODEC_REMOTEFX: return "RemoteFx";
    case CODEC_NS: return "NSCodec";
    }
    REDEMPTION_DIAGNOSTIC_POP()
}

enum {
       CODEC_GUID_IGNORE = 0,
       CODEC_GUID_NSCODEC = 1,
       CODEC_GUID_REMOTEFX = 3,
       CODEC_GUID_IMAGE_REMOTEFX = 5,
     };


// 2.2.1  NSCodec Capability Set (TS_NSCODEC_CAPABILITYSET)
// ========================================================
// The TS_NSCODEC_CAPABILITYSET structure advertises properties of the NSCodec Bitmap
// Codec. This capability set is encapsulated in the codecProperties field of the Bitmap Codec ([MS-
// RDPBCGR] section 2.2.7.2.10.1.1) structure, which is ultimately encapsulated in the Bitmap Codecs
// Capability Set ([MS-RDPBCGR] section 2.2.7.2.10), which is encapsulated in a server-to-client
// Demand Active PDU ([MS-RDPBCGR] section 2.2.1.13.1) or client-to-server Confirm Active PDU
// ([MS-RDPBCGR] section 2.2.1.13.2).

//  fAllowDynamicFidelity (1 byte): An 8-bit unsigned integer that indicates support for lossy
//  bitmap compression by reducing color fidelity ([MS-RDPEGDI] section 3.1.9.1.4).

//    FALSE = 0x00
//    TRUE = 0x01

// fAllowSubsampling (1 byte): An 8-bit unsigned integer that indicates support for chroma
//    subsampling ([MS-RDPEGDI] section 3.1.9.1.3).

//    FALSE = 0x00
//    TRUE = 0x01

// colorLossLevel (1 byte): An 8-bit unsigned integer that indicates the maximum supported
//    Color Loss Level ([MS-RDPEGDI] section 3.1.9.1.4). This value MUST be between 1 and 7
//    (inclusive).

// 2.2.1.2 TS_RFX_SRVR_CAPS_CONTAINER

// The TS_RFX_SRVR_CAPS_CONTAINER structure is the top-level server capability
// container, which is sent from the server to the client. It is encapsulated
// in the codecProperties field of the Bitmap Codec structure ([MS-RDPBCGR]
// section 2.2.7.2.10.1.1), which is ultimately encapsulated in the Bitmap
// Codecs Capability Set ([MS-RDPBCGR] section 2.2.7.2.10). The Bitmap Codecs
// Capability Set is encapsulated in a server-to-client Demand Active PDU
//  ([MS-RDPBCGR] section 2.2.1.13.1).

// reserved (variable): A variable-sized array of bytes.
// All the bytes in this field MUST be set to 0. The size of the field is given
// by the corresponding codecPropertiesLength field of the parent
// TS_BITMAPCODEC, as specified in [MS-RDPBCGR] section 2.2.7.2.10.1.1 Bitmap
// Codecs Capability Set.

struct RFXICap
{
    enum {
       CT_TILE_64X64 = 0x40
    };

    uint16_t version{CLW_VERSION_1_0};          // MUST be set to 0x0100 CLW_VERSION_1_0
    uint16_t tileSize{CT_TILE_64X64};           // MUST be set to CT_TILE_64x64 (0x0040)
    uint8_t  flags{0};                          // flag from enum
    uint8_t  colConvBits{CLW_COL_CONV_ICT};     // MUST be set to CLW_COL_CONV_ICT (0x1)
    uint8_t  transformBits{CLW_XFORM_DWT_53_A}; // MUST be set to CLW_COL_CONV_ICT (0x1)
    uint8_t  entropyBits{CLW_ENTROPY_RLGR1};    // MUST be set to one of the following values :
                                                //   - CLW_ENTROPY_RLGR1
                                                //   - CLW_ENTROPY_RLGR3

    RFXICap() = default;

    void recv(InStream & stream, uint16_t len) {

        if (len < 7) {
            LOG(LOG_ERR, "Truncated RFXICap, needs=7 remains=%u", len);
            throw Error(ERR_MCS_PDU_TRUNCATED);
        }

        this->version = stream.in_uint16_le();
        if (this->version != CLW_VERSION_1_0) {
            LOG(LOG_ERR, "RFXICap expecting version=1.0");
        }

        this->tileSize = stream.in_uint16_le();
        if (this->tileSize != CT_TILE_64X64) {
            LOG(LOG_ERR, "RFXICap expecting tileSize=64x64");
        }
        this->flags = stream.in_uint8();
        this->colConvBits = stream.in_uint8();
        if (this->colConvBits != CLW_COL_CONV_ICT) {
            LOG(LOG_ERR, "RFXICap expecting colConvBits=CLW_COL_CONV_ICT");
        }
        this->transformBits = stream.in_uint8();
        if (this->transformBits != CLW_XFORM_DWT_53_A) {
            LOG(LOG_ERR, "RFXICap expecting transformBits=CLW_XFORM_DWT_53_A");
        }

        this->entropyBits = stream.in_uint8();
        switch(this->entropyBits) {
        case CLW_ENTROPY_RLGR1:
        case CLW_ENTROPY_RLGR3:
            break;
        default:
            LOG(LOG_ERR, "RFXICap unknown entropyBits");
            break;
        }
    }

    void emit(OutStream & out) const {
        out.out_uint16_le(this->version);
        out.out_uint16_le(this->tileSize);
        out.out_uint8(this->flags);
        out.out_uint8(this->colConvBits);
        out.out_uint8(this->transformBits);
        out.out_uint8(this->entropyBits);
    }

    [[nodiscard]]
    static size_t computeSize() {
        return 8;
    }
};

// 2.2.1.1.1.1 TS_RFX_CAPSET
// =========================
// The TS_RFX_CAPSET structure contains the capability information specific to the RemoteFX codec. It
// contains a variable number of TS_RFX_ICAP (section 2.2.1.1.1.1.1) structures that are used to
// configure the encoder state.

// blockType (2 bytes): A 16-bit, unsigned integer. Specifies the data block type. This field MUST
//    be set to CBY_CAPSET (0xCBC1).
//
// blockLen (4 bytes): A 32-bit, unsigned integer. Specifies the combined size, in bytes, of the
//    blockType, blockLen, codecId, capsetType, numIcaps, icapLen, and icapsData fields.
//
// codecId (1 byte): An 8-bit, unsigned integer. Specifies the codec ID. This field MUST be set to
//    0x01.
// capsetType (2 bytes): A 16-bit, unsigned integer. This field MUST be set to CLY_CAPSET
//    (0xCFC0).
//
// numIcaps (2 bytes): A 16-bit, unsigned integer. The number of TS_RFX_ICAP structures
//    contained in the icapsData field.
//
// icapLen (2 bytes): A 16-bit, unsigned integer. Specifies the size, in bytes, of each
//    TS_RFX_ICAP structure contained in the icapsData field.
//
// icapsData (variable): A variable-length array of TS_RFX_ICAP (section 2.2.1.1.1.1.1)
//    structures. Each structure MUST be packed on byte boundaries. The size of each
//    TS_RFX_ICAP structure within the array is specified in the icapLen field.



enum {
    CAPLEN_BITMAP_CODECS_CAPS = 5
};

struct Emit_CS_BitmapCodec
{
    uint8_t  codecGUID[16] {}; // 16 bits array filled with fixed lists of values
    uint8_t  codecID{0};  // CS : a bitmap data identifier code
                          // SC :
                          //    - if codecGUID == CODEC_GUID_NSCODEC, MUST be set to 1
    uint16_t codecPropertiesLength{0}; // size in bytes of the next field

    BitmapCodecType codecType{CODEC_UNKNOWN};

    enum {
        RFXNOCAPS,
        RFXCLNTCAPS
    };

    uint8_t CodecPropertiesId;

    Emit_CS_BitmapCodec() = default;

    void setCodecGUID(uint8_t codecGUID)
    {
        this->codecPropertiesLength = 0;
        switch(codecGUID) {
        case CODEC_GUID_NSCODEC:
            memcpy(this->codecGUID, "\xB9\x1B\x8D\xCA\x0F\x00\x4F\x15\x58\x9F\xAE\x2D\x1A\x87\xE2\xD6", 16);
            this->codecID = 1;
            this->codecType = CODEC_NS;
            this->CodecPropertiesId = RFXNOCAPS;
            break;
        case CODEC_GUID_REMOTEFX:
            memcpy(this->codecGUID, "\x12\x2F\x77\x76\x72\xBD\x63\x44\xAF\xB3\xB7\x3C\x9C\x6F\x78\x86", 16);
            this->CodecPropertiesId = RFXCLNTCAPS;
            this->codecType = CODEC_REMOTEFX;
            this->codecPropertiesLength = 49;
            break;
        case CODEC_GUID_IMAGE_REMOTEFX:
            memcpy(this->codecGUID, "\xD4\xCC\x44\x27\x8A\x9D\x74\x4E\x80\x3C\x0E\xCB\xEE\xA1\x9C\x54", 16);
            this->CodecPropertiesId = RFXCLNTCAPS;
            this->codecType = CODEC_REMOTEFX;
            this->codecPropertiesLength = 49;
            break;
        default:
            memset(this->codecGUID, 0, 16);
            this->codecType = CODEC_UNKNOWN;
            this->CodecPropertiesId = RFXNOCAPS;
            break;
        }
    }

    void emit(OutStream & out) const {
        out.out_copy_bytes(this->codecGUID, 16);
        out.out_uint8(this->codecID);
        out.out_uint16_le(this->codecPropertiesLength);

        //byte_ptr props = out.get_current();
        if (this->CodecPropertiesId ==RFXCLNTCAPS){
            uint32_t length{49};
            uint32_t captureFlags{CARDP_CAPS_CAPTURE_NON_CAC};
            uint32_t capsLength{37};

            std::array<RFXICap, 2> icapsData{};
            icapsData[0].entropyBits = CLW_ENTROPY_RLGR1;
            icapsData[1].entropyBits = CLW_ENTROPY_RLGR3;

            /* TS_RFX_CLNT_CAPS_CONTAINER */
            out.out_uint32_le(length);
            out.out_uint32_le(captureFlags);
            out.out_uint32_le(capsLength);

            /* TS_RFX_CAPS */
            out.out_uint16_le(CBY_CAPS); /* blockType */
            out.out_uint32_le(8);        /* blockLen */
            out.out_uint16_le(1);         /* numCapsets */

            /* TS_RFX_CAPSET */
            out.out_uint16_le(CBY_CAPSET); /* blockType */
            out.out_uint32_le(29); /* blockLen */
            out.out_uint8(0x01); /* codecId (MUST be set to 0x01) */
            out.out_uint16_le(CLY_CAPSET); /* capsetType */
            out.out_uint16_le(icapsData.size()); /* numIcaps */
            out.out_uint16_le(8); /* icapLen */

            for (RFXICap const& cap : icapsData) {
                cap.emit(out);
            }
        }
    }

    size_t computeSize() const {
        return 19u + this->codecPropertiesLength;
    }

    void log() const {
        unsigned long sz = this->codecPropertiesLength;
        LOG(LOG_INFO, "  -%s, id=%d, size=%lu codecProperties=%lu",
                bitmapCodecTypeStr(this->codecType),
                this->codecID, 19 + sz, sz);
    }
};

/** @brief BitmapCodec capabilities */
struct Emit_CS_BitmapCodecCaps : public Capability {

    Emit_CS_BitmapCodec bitmapCodecArray[BITMAPCODECS_MAX_SIZE] {};
    uint8_t bitmapCodecCount{0};

    bool haveRemoteFxCodec{false};
    uint8_t codecCounter{2};

    Emit_CS_BitmapCodecCaps()
    : Capability(CAPSETTYPE_BITMAP_CODECS, CAPLEN_BITMAP_CODECS_CAPS)
    {}

    void addCodec(uint8_t codec_id, uint8_t codecType) {
        switch(codecType) {
        case CODEC_GUID_REMOTEFX:
        case CODEC_GUID_IMAGE_REMOTEFX: {
            Emit_CS_BitmapCodec *codec = &this->bitmapCodecArray[this->bitmapCodecCount];
            codec->setCodecGUID(codecType);
            codec->codecID = codec_id;
            this->haveRemoteFxCodec = true;
            this->bitmapCodecCount++;
            this->codecCounter++;
            if (this->codecCounter == 1) { /* reserved for NSCodec */
                this->codecCounter++;
            }
            break;
        }
        case CODEC_GUID_NSCODEC:
            /* TODO */
        default:
            LOG(LOG_ERR, "unsupported codecType=%u", codecType);
            break;
        }

        this->len = CAPLEN_BITMAP_CODECS_CAPS + this->computeCodecsSize();
    }

    size_t computeCodecsSize() {
        size_t codecsLen = 0;
        for (int i = 0; i < this->bitmapCodecCount; i++) {
            codecsLen += this->bitmapCodecArray[i].computeSize();
        }

        return codecsLen;
    }

    void emit(OutStream & out) {
        out.out_uint16_le(this->capabilityType);
        out.out_uint16_le(this->len);

        out.out_uint8(this->bitmapCodecCount);

        for (int i = 0; i < this->bitmapCodecCount; i++) {
            this->bitmapCodecArray[i].emit(out);
        }
    }

    void log(const char * msg) const {
        LOG(LOG_INFO, "%s Emit_CS_BitmapCodecCaps [Client to Server]", msg);
        LOG(LOG_INFO, "Emit_CS_BitmapCodecCaps (%u bytes)", this->len);
        LOG(LOG_INFO, "Emit_CS_BitmapCodecCaps::BitmapCodec::bitmapCodecCount %u", this->bitmapCodecCount);

        for (auto i = 0; i < this->bitmapCodecCount; i++) {
            this->bitmapCodecArray[i].log();
        }
    }
};


struct Recv_CS_BitmapCodec
{
    uint8_t  codecGUID[16] {}; // 16 bits array filled with fixed lists of values
    uint8_t  codecID{0};  // CS : a bitmap data identifier code
                          // SC :
                          //    - if codecGUID == CODEC_GUID_NSCODEC, MUST be set to 1
    uint16_t codecPropertiesLength{0}; // size in bytes of the next field

    /** @brief TS_RFX_CLNT_CAPS_CONTAINER */
    struct Recv_RFXClntCaps
    {
        uint32_t length{49};
        uint32_t captureFlags{CARDP_CAPS_CAPTURE_NON_CAC};
        uint32_t capsLength{37};

        std::vector<RFXICap> icapsData{RFXICap{}};

        Recv_RFXClntCaps() {
            icapsData.resize(2);
            icapsData[0].entropyBits = CLW_ENTROPY_RLGR1;
            icapsData[1].entropyBits = CLW_ENTROPY_RLGR3;
        }

        void recv(InStream & stream, uint16_t len)
        {
            hexdump(stream.get_current(), len);
            if (len < 12) {
                LOG(LOG_ERR, "Truncated RFXClntCaps, need=%u remains=%zu", this->capsLength, stream.in_remain());
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            this->length = stream.in_uint32_le();
            this->captureFlags = stream.in_uint32_le();
            this->capsLength = stream.in_uint32_le();
    //        LOG(LOG_INFO, "length=%.4X flags=%.4X capsLen=%.4X", this->length, this->captureFlags, this->capsLength);

            if (len < this->capsLength + 12) {
                LOG(LOG_ERR, "Truncated RFXClntCaps, need=%u remains=%zu", this->capsLength, stream.in_remain());
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            /* TS_RFX_CAPS */
            uint16_t blockType = stream.in_uint16_le();
            uint32_t blockLen = stream.in_uint32_le();
            uint16_t numCapsets = stream.in_uint16_le();
    //        LOG(LOG_INFO, "blockType=%.4X blockLen=%.4X numCapsets=%.4X", blockType, blockLen, numCapsets);

            if (blockType != CBY_CAPS) {
                LOG(LOG_ERR, "we want blockType == CBY_CAPS");
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            if (blockLen != 8) {
                LOG(LOG_ERR, "we want blockLen == 8");
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            if (numCapsets != 1) {
                LOG(LOG_ERR, "we want numCapsets == 1");
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            /* TS_RFX_CAPSET */
            {
                uint16_t blockType = stream.in_uint16_le();
                /*uint32_t blockLen = */stream.in_uint32_le();
                uint8_t codecId = stream.in_uint8();
                uint16_t capsetType = stream.in_uint16_le();
                uint16_t numIcaps = stream.in_uint16_le();
                uint16_t icapLen = stream.in_uint16_le();

    //            LOG(LOG_INFO, "blockType=%.4X blockLen=%.4X codecId=%.2X", blockType, blockLen, codecId);

                if (blockType != CBY_CAPSET) {
                    LOG(LOG_ERR, "we want blockType == CBY_CAPSET");
                    throw Error(ERR_MCS_PDU_TRUNCATED);
                }

                if (codecId != 0x1) {
                    LOG(LOG_ERR, "we want codecId == 1");
                    throw Error(ERR_MCS_PDU_TRUNCATED);
                }

                if (capsetType != CLY_CAPSET) {
                    LOG(LOG_ERR, "we want capsetType == CLY_CAPSET");
                    throw Error(ERR_MCS_PDU_TRUNCATED);
                }

                ::check_throw(stream, numIcaps * icapLen, "not enough room for RFXIcaps", ERR_MCS_PDU_TRUNCATED);

                this->icapsData.resize(2);

                // TODO this->icapsData.size() == 2, but loop from 0 to numIcaps
                for (int i = 0; i < numIcaps; i++) {
                    this->icapsData[i].recv(stream, icapLen);
                }
            }
        }

        [[nodiscard]] size_t computeSize() const {
            return this->length;
        }
    };

    struct Recv_NSCodecCaps
    {
        uint8_t fAllowDynamicFidelity{0}; // true/false
        uint8_t fAllowSubsampling{0};     // true/false
        uint8_t colorLossLevel{1};        // Between 1 and 7

        Recv_NSCodecCaps() = default;

        void recv(InStream & stream, uint16_t len)
        {
            size_t expected = 3;
            if (len < 3) {
                LOG(LOG_ERR, "Truncated NSCodecCaps, needs=%zu remains=%u", expected, len);
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            this->fAllowDynamicFidelity = stream.in_uint8();
            this->fAllowSubsampling = stream.in_uint8();
            this->colorLossLevel = stream.in_uint8();
        }

        [[nodiscard]]
        static size_t computeSize()
        {
            return 3;
        }
    };

    BitmapCodecType codecType{CODEC_UNKNOWN};

    Recv_CS_BitmapCodec() = default;

    [[nodiscard]] size_t computeSize() const {
        switch (this->codecType){
        case CODEC_NS:
            return 19u + Recv_NSCodecCaps().computeSize();
        case CODEC_REMOTEFX:
            return 19u + Recv_RFXClntCaps().computeSize();
        case CODEC_IGNORE:
        case CODEC_UNKNOWN:
            break;
        }
        return 19u;
    }

    void recv(InStream & stream) {
        ::check_throw(stream, 19, "Recv_CS_BitmapCodec::recv", ERR_MCS_PDU_TRUNCATED);

        stream.in_copy_bytes(codecGUID, 16);
        this->codecID = stream.in_uint8();
        this->codecPropertiesLength = stream.in_uint16_le();

        ::check_throw(stream, this->codecPropertiesLength, "codec properties in CS_BitmapCodec", ERR_MCS_PDU_TRUNCATED);

        if (memcmp(codecGUID, "\xB9\x1B\x8D\xCA\x0F\x00\x4F\x15\x58\x9F\xAE\x2D\x1A\x87\xE2\xD6", 16) == 0) {
            /* CODEC_GUID_NSCODEC */
            this->codecType = CODEC_NS;
            Recv_NSCodecCaps().recv(stream, this->codecPropertiesLength);
        } else if (memcmp(codecGUID, "\x12\x2F\x77\x76\x72\xBD\x63\x44\xAF\xB3\xB7\x3C\x9C\x6F\x78\x86", 16) == 0) {
            /* CODEC_GUID_IMAGE_REMOTEFX */
            this->codecType = CODEC_REMOTEFX;
            Recv_RFXClntCaps().recv(stream, this->codecPropertiesLength);
        } else if(memcmp(codecGUID, "\xD4\xCC\x44\x27\x8A\x9D\x74\x4E\x80\x3C\x0E\xCB\xEE\xA1\x9C\x54", 16) == 0) {
            /* CODEC_GUID_IMAGE_REMOTEFX */
            this->codecType = CODEC_REMOTEFX;
            Recv_RFXClntCaps().recv(stream, this->codecPropertiesLength);
        } else if (memcmp(codecGUID, "\xA6\x51\x43\x9C\x35\x35\xAE\x42\x91\x0C\xCD\xFC\xE5\x76\x0B\x58", 16) == 0) {
            /* CODEC_REMOTEFX */
            this->codecType = CODEC_IGNORE;
            stream.in_skip_bytes(this->codecPropertiesLength);
        } else {
            this->codecType = CODEC_UNKNOWN;
            stream.in_skip_bytes(this->codecPropertiesLength);
            LOG(LOG_ERR, "unknown codec");
        }
    }

    void log() const {
        long unsigned cs = this->computeSize();
        LOG(LOG_INFO, "  -%s, id=%d, size=%lu codecProperties=%lu",
                bitmapCodecTypeStr(this->codecType), this->codecID, cs, cs-19u);
    }
};


/** @brief BitmapCodec capabilities */
struct Recv_CS_BitmapCodecCaps : public Capability {

    Recv_CS_BitmapCodec bitmapCodecArray[BITMAPCODECS_MAX_SIZE] {};
    uint8_t bitmapCodecCount{0};

    bool haveRemoteFxCodec{false};

    Recv_CS_BitmapCodecCaps()
    : Capability(CAPSETTYPE_BITMAP_CODECS, CAPLEN_BITMAP_CODECS_CAPS)
    {}

    void recv(InStream & stream, uint16_t len) {
        this->len = len;

        unsigned expected = 1;
        if (this->len < expected){
            LOG(LOG_ERR, "Truncated BitmapCodecs, need=%u remains=%hu", expected, this->len);
            throw Error(ERR_MCS_PDU_TRUNCATED);
        }

        this->bitmapCodecCount = stream.in_uint8();

        InStream subStream({stream.get_current(), len-5u});

        for (int i = 0; i < this->bitmapCodecCount; i++) {
            this->bitmapCodecArray[i].recv(subStream);

            if (this->bitmapCodecArray[i].codecType == CODEC_REMOTEFX) {
                this->haveRemoteFxCodec = true;
            }
        }
    }

    void log(const char * msg) const {
        LOG(LOG_INFO, "%s Recv_CS_BitmapCodecCaps [Client to Server]", msg);
        LOG(LOG_INFO, "Recv_CS_BitmapCodecCaps (%u bytes)", this->len);
        LOG(LOG_INFO, "Recv_CS_BitmapCodecCaps::BitmapCodec::bitmapCodecCount %u", this->bitmapCodecCount);

        for (auto i = 0; i < this->bitmapCodecCount; i++) {
            this->bitmapCodecArray[i].log();
        }
    }
};


struct Emit_SC_BitmapCodec
{
    uint8_t  codecGUID[16] {}; // 16 bits array filled with fixed lists of values
    uint8_t  codecID{0};  // CS : a bitmap data identifier code
                          // SC :
                          //    - if codecGUID == CODEC_GUID_NSCODEC, MUST be set to 1
    uint16_t codecPropertiesLength{0}; // size in bytes of the next field


    struct Emit_RFXSrvrCaps
    {
        uint16_t len{0};

        Emit_RFXSrvrCaps() = default;

        void emit(OutStream & out_stream) const
        {
            out_stream.out_clear_bytes(this->len);
        }

        [[nodiscard]] size_t computeSize() const
        {
            return this->len;
        }
    };

    struct Emit_NSCodecCaps
    {
        uint8_t fAllowDynamicFidelity{0}; // true/false
        uint8_t fAllowSubsampling{0};     // true/false
        uint8_t colorLossLevel{1};        // Between 1 and 7

        Emit_NSCodecCaps() = default;

        void emit(OutStream & out) const
        {
            out.out_uint8(this->fAllowDynamicFidelity);
            out.out_uint8(this->fAllowSubsampling);
            out.out_uint8(this->colorLossLevel);
        }

        [[nodiscard]]
        static size_t computeSize()
        {
            return 3;
        }
    };

    BitmapCodecType codecType{CODEC_UNKNOWN};

    struct RFXNoCaps
    {
        static void emit(OutStream & /*out*/) {}
        static void recv(InStream & /*stream*/, uint16_t /*len*/) {}
        [[nodiscard]] static size_t computeSize() { return 0; }
    };

    struct CodecProperties
    {
        void emit(OutStream & out) const
        {
            auto f = [&](auto& caps) -> void { caps.emit(out); };
            std::visit(f, this->interface);
        }

        [[nodiscard]] size_t computeSize() const
        {
            auto f = [](auto& caps) -> size_t { return caps.computeSize(); };
            return std::visit(f, this->interface);
        }

        std::variant<RFXNoCaps, Emit_RFXSrvrCaps, Emit_NSCodecCaps> interface;
    };

    CodecProperties codecProperties;

    Emit_SC_BitmapCodec() = default;

    void setCodecGUID(uint8_t codecGUID)
    {
        switch(codecGUID) {
        case CODEC_GUID_NSCODEC:
            memcpy(this->codecGUID, "\xB9\x1B\x8D\xCA\x0F\x00\x4F\x15\x58\x9F\xAE\x2D\x1A\x87\xE2\xD6", 16);
            this->codecID = 1;
            this->codecType = CODEC_NS;
            this->codecProperties.interface = RFXNoCaps{};
            break;
        case CODEC_GUID_REMOTEFX:
            memcpy(this->codecGUID, "\x12\x2F\x77\x76\x72\xBD\x63\x44\xAF\xB3\xB7\x3C\x9C\x6F\x78\x86", 16);
            this->codecProperties.interface = Emit_RFXSrvrCaps();
            this->codecType = CODEC_REMOTEFX;
            break;

        case CODEC_GUID_IMAGE_REMOTEFX:
            memcpy(this->codecGUID, "\xD4\xCC\x44\x27\x8A\x9D\x74\x4E\x80\x3C\x0E\xCB\xEE\xA1\x9C\x54", 16);
            this->codecProperties.interface = Emit_RFXSrvrCaps();
            this->codecType = CODEC_REMOTEFX;
            break;
        default:
            memset(this->codecGUID, 0, 16);
            this->codecType = CODEC_UNKNOWN;
            this->codecProperties.interface = RFXNoCaps{};
            break;
        }

        this->codecPropertiesLength = this->codecProperties.computeSize();
    }

    [[nodiscard]] size_t computeSize() const {
        return 19u + this->codecProperties.computeSize();
    }

    void emit(OutStream & out) const {
        out.out_copy_bytes(this->codecGUID, 16);
        out.out_uint8(0);
        out.out_uint16_le(this->codecPropertiesLength);

        //byte_ptr props = out.get_current();
        this->codecProperties.emit(out);

        //LOG(LOG_ERR, "codecProperties:");
        //hexdump(props, this->codecPropertiesLength);
    }

    void log() const {
        LOG(LOG_INFO, "  -%s, id=%d, size=%lu codecProperties=%lu",
                bitmapCodecTypeStr(this->codecType),
                this->codecID, this->computeSize(), this->codecProperties.computeSize());
    }
};

/** @brief BitmapCodec capabilities */
struct Emit_SC_BitmapCodecCaps : public Capability {

    Emit_SC_BitmapCodec bitmapCodecArray[BITMAPCODECS_MAX_SIZE] {};
    uint8_t bitmapCodecCount{0};

    bool clientMode = false;
    bool haveRemoteFxCodec{false};
    uint8_t codecCounter{2};

    Emit_SC_BitmapCodecCaps()
    : Capability(CAPSETTYPE_BITMAP_CODECS, CAPLEN_BITMAP_CODECS_CAPS)
    {}

    private:
    uint8_t addCodec(uint8_t codecType) {
        uint8_t ret = 1;

        switch(codecType) {
        case CODEC_GUID_REMOTEFX:
        case CODEC_GUID_IMAGE_REMOTEFX: {
            Emit_SC_BitmapCodec *codec = &this->bitmapCodecArray[this->bitmapCodecCount];
            codec->setCodecGUID(codecType);
            ret = codec->codecID = this->codecCounter;
            this->haveRemoteFxCodec = true;
            this->bitmapCodecCount++;
            this->codecCounter++;
            if (this->codecCounter == 1) { /* reserved for NSCodec */
                this->codecCounter++;
            }
            break;
        }
        case CODEC_GUID_NSCODEC:
            /* TODO */
        default:
            LOG(LOG_ERR, "unsupported codecType=%u", codecType);
            break;
        }

        len = CAPLEN_BITMAP_CODECS_CAPS + computeCodecsSize();
        return ret;
    }

    public:
    size_t computeCodecsSize() {
        size_t codecsLen = 0;
        for (int i = 0; i < this->bitmapCodecCount; i++) {
            codecsLen += this->bitmapCodecArray[i].computeSize();
        }

        return codecsLen;
    }

    void emit(OutStream & out, array_view<uint8_t> supported_codecs)
    {
        for (auto codec: supported_codecs){
            this->addCodec(codec);
        }

        out.out_uint16_le(this->capabilityType);
        out.out_uint16_le(this->len);

        out.out_uint8(this->bitmapCodecCount);

        for (int i = 0; i < this->bitmapCodecCount; i++) {
            this->bitmapCodecArray[i].emit(out);
        }
    }

    void log(const char * msg) const {
        LOG(LOG_INFO, "%s Emit_SC_BitmapCodecCaps [Server to Client]", msg);
        LOG(LOG_INFO, "Emit_SC_BitmapCodecCaps (%u bytes)", this->len);
        LOG(LOG_INFO, "Emit_SC_BitmapCodecCaps::BitmapCodecs::bitmapCodecCount %u", this->bitmapCodecCount);

        for (auto i = 0; i < this->bitmapCodecCount; i++) {
            this->bitmapCodecArray[i].log();
        }
    }
};


struct Recv_SC_BitmapCodec
{
    uint8_t  codecGUID[16] {}; // 16 bits array filled with fixed lists of values
    uint8_t  codecID{0};  // CS : a bitmap data identifier code
                          // SC :
                          //    - if codecGUID == CODEC_GUID_NSCODEC, MUST be set to 1
    uint16_t codecPropertiesLength{0}; // size in bytes of the next field

    BitmapCodecType codecType{CODEC_UNKNOWN};

    struct Recv_RFXSrvrCaps
    {
        uint16_t len{0};

        Recv_RFXSrvrCaps() = default;

        void emit(OutStream & out_stream) const
        {
            out_stream.out_clear_bytes(this->len);
        }

        void recv(InStream & stream, uint16_t len)
        {
            stream.in_skip_bytes(len);
            this->len = len;
        }

        [[nodiscard]] size_t computeSize() const
        {
            return this->len;
        }
    };

    struct Recv_NSCodecCaps
    {
        uint8_t fAllowDynamicFidelity{0}; // true/false
        uint8_t fAllowSubsampling{0};     // true/false
        uint8_t colorLossLevel{1};        // Between 1 and 7

        Recv_NSCodecCaps() = default;

        void emit(OutStream & out) const
        {
            out.out_uint8(this->fAllowDynamicFidelity);
            out.out_uint8(this->fAllowSubsampling);
            out.out_uint8(this->colorLossLevel);
        }

        void recv(InStream & stream, uint16_t len)
        {
            size_t expected = 3;
            if (len < 3) {
                LOG(LOG_ERR, "Truncated NSCodecCaps, needs=%zu remains=%u", expected, len);
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            this->fAllowDynamicFidelity = stream.in_uint8();
            this->fAllowSubsampling = stream.in_uint8();
            this->colorLossLevel = stream.in_uint8();
        }

        [[nodiscard]]
        static size_t computeSize()
        {
            return 3;
        }
    };

    Recv_SC_BitmapCodec() = default;

    [[nodiscard]] size_t computeSize() const {
        switch (this->codecType){
        case CODEC_NS:
            return 19u + Recv_NSCodecCaps().computeSize();
        case CODEC_REMOTEFX:
            return 19u + Recv_RFXSrvrCaps().computeSize();
        case CODEC_IGNORE:
        case CODEC_UNKNOWN:
            break;
        }
        return 19u;
    }

    void recv(InStream & stream) {
        ::check_throw(stream, 19, "Recv_SC_BitmapCodec::recv", ERR_MCS_PDU_TRUNCATED);

        stream.in_copy_bytes(codecGUID, 16);
        this->codecID = stream.in_uint8();
        this->codecPropertiesLength = stream.in_uint16_le();

        ::check_throw(stream, this->codecPropertiesLength, "codec properties in Recv_SC_BitmapCodec", ERR_MCS_PDU_TRUNCATED);

        /* CODEC_GUID_NSCODEC */
        if (memcmp(codecGUID, "\xB9\x1B\x8D\xCA\x0F\x00\x4F\x15\x58\x9F\xAE\x2D\x1A\x87\xE2\xD6", 16) == 0)
        {
            this->codecType = CODEC_NS;
            Recv_NSCodecCaps().recv(stream, this->codecPropertiesLength);
        } else
        /* CODEC_GUID_REMOTEFX */
        if (memcmp(codecGUID, "\x12\x2F\x77\x76\x72\xBD\x63\x44\xAF\xB3\xB7\x3C\x9C\x6F\x78\x86", 16) == 0)
        {
            this->codecType = CODEC_REMOTEFX;
            Recv_RFXSrvrCaps().recv(stream, this->codecPropertiesLength);
        } else
        /* CODEC_GUID_IMAGE_REMOTEFX */
        if (memcmp(codecGUID, "\xD4\xCC\x44\x27\x8A\x9D\x74\x4E\x80\x3C\x0E\xCB\xEE\xA1\x9C\x54", 16) == 0)
        {
            this->codecType = CODEC_REMOTEFX;
            Recv_RFXSrvrCaps().recv(stream, this->codecPropertiesLength);
        } else
        /* CODEC_GUID_IGNORE */
        if (memcmp(codecGUID, "\xA6\x51\x43\x9C\x35\x35\xAE\x42\x91\x0C\xCD\xFC\xE5\x76\x0B\x58", 16) == 0)
        {
            this->codecType = CODEC_IGNORE;
            stream.in_skip_bytes(this->codecPropertiesLength);
        } else
        {
            this->codecType = CODEC_UNKNOWN;
            stream.in_skip_bytes(this->codecPropertiesLength);
            LOG(LOG_ERR, "unknown codec");
        }
    }

    void log() const {
        unsigned long sz = this->computeSize();
        LOG(LOG_INFO, "  -%s, id=%d, size=%lu codecProperties=%lu",
                bitmapCodecTypeStr(this->codecType),
                this->codecID, sz, sz-19u);
    }
};



/** @brief BitmapCodec capabilities */
struct Recv_SC_BitmapCodecCaps : public Capability {

    Recv_SC_BitmapCodec bitmapCodecArray[BITMAPCODECS_MAX_SIZE] {};
    uint8_t bitmapCodecCount{0};

    bool clientMode = false;
    bool haveRemoteFxCodec{false};
    uint8_t codecCounter{2};

    Recv_SC_BitmapCodecCaps()
    : Capability(CAPSETTYPE_BITMAP_CODECS, CAPLEN_BITMAP_CODECS_CAPS)
    {}

    void recv(InStream & stream, uint16_t len) {
        this->len = len;

        unsigned expected = 1;
        if (this->len < expected){
            LOG(LOG_ERR, "Truncated BitmapCodecs, need=%u remains=%hu", expected, this->len);
            throw Error(ERR_MCS_PDU_TRUNCATED);
        }

        this->bitmapCodecCount = stream.in_uint8();

        InStream subStream({stream.get_current(), len-5u});

        for (int i = 0; i < this->bitmapCodecCount; i++) {
            this->bitmapCodecArray[i].recv(subStream);

            if (this->bitmapCodecArray[i].codecType == CODEC_REMOTEFX) {
                this->haveRemoteFxCodec = true;
            }
        }
    }

    void log(const char * msg) const {
        LOG(LOG_INFO, "%s Recv_SC_BitmapCodecCaps [Server to Client]", msg);
        LOG(LOG_INFO, "Recv_SC_BitmapCodecCaps (%u bytes)", this->len);
        LOG(LOG_INFO, "Recv_SC_BitmapCodecCaps::BitmapCodecs::bitmapCodecCount %u", this->bitmapCodecCount);

        for (auto i = 0; i < this->bitmapCodecCount; i++) {
            this->bitmapCodecArray[i].log();
        }
    }
};
