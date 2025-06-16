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
   Copyright (C) Wallix 2018
   Author(s): David Fort

*/

#include "remoteFx.hpp"
#include "core/RDP/dwt.hpp"
#include "core/RDP/orders/RDPSurfaceCommands.hpp"
#include "utils/log.hpp"
#include "utils/primitives/primitives.hpp"
#include "core/stream_throw_helpers.hpp"

#include <cinttypes>



/** 2.2.2.1.6 TS_RFX_RECT
 * The TS_RFX_RECT structure is used to specify a rectangle.
 *
 * x (2 bytes): A 16-bit, unsigned integer. The X-coordinate of the rectangle.
 * y (2 bytes): A 16-bit, unsigned integer. The Y-coordinate of the rectangle.
 * width (2 bytes): A 16-bit, unsigned integer. The width of the rectangle.
 * height (2 bytes): A 16-bit, unsigned integer. The height of the rectangle.
 */
void TS_RFX_RECT::recv(InStream & stream) {

    ::check_throw(stream, 8, "TS_RFX_SYNC::recv RFX_RECT", ERR_MCS_PDU_TRUNCATED);

    this->x = stream.in_uint16_le();
    this->y = stream.in_uint16_le();
    this->width = stream.in_uint16_le();
    this->height = stream.in_uint16_le();
}

void TS_RFX_RECT::send(OutStream & stream) const
{
    stream.out_uint16_le(this->x);
    stream.out_uint16_le(this->y);
    stream.out_uint16_le(this->width);
    stream.out_uint16_le(this->height);
}


/**
 * 2.2.2.2.1 TS_RFX_SYNC
 * The TS_RFX_SYNC message MUST be the first message in any encoded stream. The decoder MUST
 * examine this message to determine whether the protocol version is supported.
 *
 * BlockT (6 bytes): A TS_RFX_BLOCKT (section 2.2.2.1.1) structure. The blockType field MUST
 *         be set to WBT_SYNC (0xCCC0).
 * magic (4 bytes): A 32-bit, unsigned integer. This field MUST be set to WF_MAGIC(0xCACCACCA).
 * version (2 bytes): A 16-bit, unsigned integer. Indicates the version number. This field MUST
 *         be set to WF_VERSION_1_0 (0x0100).
 */
void TS_RFX_SYNC::recv(InStream & stream) {

    ::check_throw(stream, 6, "TS_RFX_SYNC::recv RFX_PACKET", ERR_MCS_PDU_TRUNCATED);

    uint32_t magic = stream.in_uint32_le();
    if (magic != WF_MAGIC) {
        LOG(LOG_ERR, "invalid RFX Sync magic");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    uint16_t version = stream.in_uint16_le();
    if (version != WF_VERSION_1_0) {
        LOG(LOG_ERR, "unsupported RFX Sync version");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }
}

void TS_RFX_SYNC::send(OutStream & stream) const
{
    stream.out_uint16_le(WBT_SYNC);
    stream.out_uint32_le(12);
    stream.out_uint32_le(WF_MAGIC);
    stream.out_uint16_le(WF_VERSION_1_0);
}

//TS_RFX_CODEC_VERSIONS

// 2.2.2.2.2 TS_RFX_CODEC_VERSIONS
// The TS_RFX_CODEC_VERSIONS message indicates the version of the RemoteFX codec that is being
// used.
//
// BlockT (6 bytes): A TS_RFX_BLOCKT (section 2.2.2.1.1) structure. The blockType field MUST
//        be set to WBT_CODEC_VERSIONS (0xCCC1).
// numCodecs (1 byte): An 8-bit, unsigned integer. Specifies the number of codec version data
//        blocks in the codecs field. This field MUST be set to 0x01.
// codecs (3 bytes): A TS_RFX_CODEC_VERSIONT (section 2.2.2.1.4) structure. The codecId
//        field MUST be set to 0x01 and the version field MUST be set to WF_VERSION_1_0 (0x0100).

void TS_RFX_CODEC_VERSIONS::recv(InStream & stream) {

    ::check_throw(stream, 4, "TS_RFX_CODEC_VERSIONS::recv invalid TS_RFX_CODEC_VERSIONS", ERR_MCS_PDU_TRUNCATED);

    this->numCodecs = stream.in_uint8();
    if (this->numCodecs != 1) {
        LOG(LOG_ERR, "expecting numCodecs=1 in TS_RFX_CODEC_VERSIONS");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    this->codecId = stream.in_uint8();
    if (this->codecId != 1) {
        LOG(LOG_ERR, "expecting codecId=1 in TS_RFX_CODEC_VERSIONS");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    this->version = stream.in_uint16_le();
    if (this->version != WF_VERSION_1_0) {
        LOG(LOG_ERR, "expecting version=0x100 in TS_RFX_CODEC_VERSIONS");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }
}

void TS_RFX_CODEC_VERSIONS::send(OutStream & stream) const
{
    stream.out_uint16_le(WBT_CODEC_VERSION);
    stream.out_uint32_le(10);

    stream.out_uint8(this->numCodecs);
    stream.out_uint8(this->codecId);
    stream.out_uint16_le(this->version);
}

//
// 2.2.2.1.2 TS_RFX_CODEC_CHANNELT
// The TS_RFX_CODEC_CHANNELT structure is an extension of the TS_RFX_BLOCKT structure. It is
// present as the first field in messages that are targeted for a specific combination of codec and
// channel.
//
// codecId (1 byte): An 8-bit, unsigned integer. Specifies the codec ID. This field MUST be set to
//        0x01.
// channelId (1 byte): An 8-bit, unsigned integer. Specifies the channel ID. This field MUST be
//         set to 0x00.

void TS_RFX_CODEC_CHANNELT::recv(InStream & stream) {

    ::check_throw(stream, 2, "TS_RFX_CODEC_CHANNELT::recv invalid TS_RFX_CODEC_CHANNELT", ERR_MCS_PDU_TRUNCATED);

    this->codecId = stream.in_uint8();
    this->channelId = stream.in_uint8();
}

void TS_RFX_CODEC_CHANNELT::send(OutStream & stream) const
{
    stream.out_uint8(this->codecId);
    stream.out_uint8(this->channelId);
}

// 2.2.2.2.4 TS_RFX_CONTEXT
// The TS_RFX_CONTEXT message contains information regarding the encoding properties being used.
// CodecChannelT (8 bytes): A TS_RFX_CODEC_CHANNELT structure. The blockType field MUST be set to WBT_CONTEXT (0xCCC3).
// ctxId (1 byte): An 8-bit unsigned integer. Specifies an identifier for this context message. This
//        field MUST be set to 0x00. The decoder SHOULD ignore this field.
// tileSize (2 bytes): A 16-bit unsigned integer. Specifies the tile size used by the RemoteFX
//        codec. This field MUST be set to CT_TILE_64x64 (0x0040), indicating that a tile is 64 x 64 pixels.
// properties (2 bytes): A 16-bit unsigned integer. Contains a collection of bit-packed property
//        fields. The format of this field is described by the following bitfield diagram.
//  +-------------------------------+
//  |     |   |       |  1 1 1|1 1|1|
//  |0 1 2|3 4|5 6 7 8|9 0 1 2|3 4|5|
//  +-------------------------------+
//  |flags|cct|  xft  |  et   | qt|r|
//  +-------------------------------+
//  flags (3 bits): A 3-bit unsigned integer. Specifies operational flags.
//        Flag Meaning
//            CODEC_MODE 0x02 - The codec is operating in image mode. If this flag is not set, the codec is
//                            operating in video mode.
//        When operating in image mode, the Encode Headers messages (section 2.2.2.2) MUST
//        always precede an encoded frame. When operating in video mode, the header messages
//        MUST be present at the beginning of the stream and MAY be present elsewhere.
//    cct (2 bits): A 2-bit unsigned integer. Specifies the color conversion transform. This field
//        MUST be set to COL_CONV_ICT (0x1) to specify the transform defined by the equations
//        in sections 3.1.8.1.3 and 3.1.8.2.5. The decoder SHOULD ignore this field.
//    xft (4 bits): A 4-bit unsigned integer. Specifies the DWT. This field MUST be set to
//        CLW_XFORM_DWT_53_A (0x1), which indicates the DWT given by the equations in
//        sections 3.1.8.1.4 and 3.1.8.2.4.
//    et (4 bits): A 4-bit unsigned integer. Specifies the entropy algorithm. This field MUST be
//        set to one of the following values.
//            CLW_ENTROPY_RLGR1(0x01) RLGR algorithm as detailed in 3.1.8.1.7.1.
//            CLW_ENTROPY_RLGR3(0x04) RLGR algorithm as detailed in 3.1.8.1.7.2.
//        The decoder SHOULD ignore this value and use the value defined in the properties field
//        of TS_RFX_TILESET (section 2.2.2.3.4) (section 2.2.2.3.4).
//    qt (2 bits): A 2-bit unsigned integer. Specifies the quantization type. This field MUST be
//        set to SCALAR_QUANTIZATION (0x1). The decoder SHOULD ignore this field.
//    r (1 bit): A 1-bit field reserved for future use. This field MUST be ignored when received.


void TS_RFX_CONTEXT::recv(InStream & stream) {
    TS_RFX_CODEC_CHANNELT::recv(stream);

    ::check_throw(stream, 5, "TS_RFX_CONTEXT::recv invalid TS_RFX_CONTEXT", ERR_MCS_PDU_TRUNCATED);

    this->ctxId = stream.in_uint8();
    this->tileSize = stream.in_uint16_le();
    if (this->tileSize != CT_TILE_64X64) {
        LOG(LOG_ERR, "only a tileSize of 64x64 is supported");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }
    this->properties = stream.in_uint16_le();
}

void TS_RFX_CONTEXT::send(OutStream & stream) const
{
    stream.out_uint16_le(WBT_CONTEXT);
    stream.out_uint32_le(13);

    TS_RFX_CODEC_CHANNELT::send(stream);

    stream.out_uint8(this->ctxId);
    stream.out_uint16_le(this->tileSize);
    stream.out_uint16_le(this->properties);
}

// 2.2.2.1.3 TS_RFX_CHANNELT
// The TS_RFX_CHANNELT structure is used to specify the screen resolution of a channel.
//
// channelId (1 byte): An 8-bit, unsigned integer. Specifies the identifier of the channel. This
//         field MUST be set to 0x00.
// width (2 bytes): A 16-bit, signed integer. Specifies the frame width of the channel. This field
//        SHOULD<3> be within the range of 1 to 4096 (inclusive).
// height (2 bytes): A 16-bit, signed integer. Specifies the frame height of the channel. This field
//         SHOULD<4> be within the range of 1 to 2048 (inclusive).

void TS_RFX_CHANNELT::recv(InStream & stream) {

    ::check_throw(stream, 5, "TS_RFX_CHANNELT::recv invalid TS_RFX_CHANNELT", ERR_MCS_PDU_TRUNCATED);

    this->channelId = stream.in_uint8();
    if (this->channelId != 0){
        LOG(LOG_ERR, "invalid TS_RFX_CHANNELT channelId");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    this->width = stream.in_uint16_le();
    this->height = stream.in_uint16_le();
}

void TS_RFX_CHANNELT::send(OutStream & stream) const
{
    stream.out_uint8(this->channelId);
    stream.out_uint16_le(this->width);
    stream.out_uint16_le(this->height);
}

// 2.2.2.2.3 TS_RFX_CHANNELS
// The TS_RFX_CHANNELS message contains the list of channels. Each active monitor on the server
// must correspond to an entry in this list. The list can have more entries than the number of active
// monitors. The decoder endpoint MUST be able to support channels with different frame dimensions.
//
// BlockT (6 bytes): A TS_RFX_BLOCKT (section 2.2.2.1.1) structure. The blockType field MUST
//         be set to WBT_CHANNELS (0xCCC2).
// numChannels (1 byte): An 8-bit, unsigned integer. Specifies the number of channel data
//        blocks in the channels field.
// Channels (variable): A variable-length array of TS_RFX_CHANNELT (section 2.2.2.1.3)
//        structures. The number of elements in this array is specified in the numChannels field.

TS_RFX_CHANNELS::~TS_RFX_CHANNELS() {
    delete[] this->channels;
}

void TS_RFX_CHANNELS::recv(InStream & stream) {

    ::check_throw(stream, 1, "TS_RFX_CHANNELS::recv invalid TS_RFX_CHANNELS", ERR_MCS_PDU_TRUNCATED);

    this->numChannels = stream.in_uint8();
    if (this->numChannels) {
        this->channels = new TS_RFX_CHANNELT[numChannels]();
        for(int i = 0; i < this->numChannels; i++) {
            this->channels[i].recv(stream);
        }
    }
}

void TS_RFX_CHANNELS::setChannel(uint8_t channelId, uint16_t width, uint16_t height) {
    this->channels = new TS_RFX_CHANNELT[1]();
    this->channels->channelId = channelId;
    this->channels->width = width;
    this->channels->height = height;
}

void TS_RFX_CHANNELS::send(OutStream & stream) const
{
    stream.out_uint16_le(WBT_CHANNELS);
    stream.out_uint16_le(6 + this->numChannels * 6);

    stream.out_uint8(this->numChannels); /* numChannels (1 byte) */
    for (int i = 0; i < this->numChannels; i++){
        this->channels[i].send(stream);
    }
}

//
// 2.2.2.3.1 TS_RFX_FRAME_BEGIN
// The TS_RFX_FRAME_BEGIN message indicates the start of a new frame for a specific channel in the
// encoded stream.
//
//    frameIdx (4 bytes): A 32-bit unsigned integer. Specifies the index of the frame in the current
//        video sequence. This field is used when the codec is operating in video mode, as specified
//        using the flags field of the TS_RFX_CONTEXT (section 2.2.2.2.4) message. If the codec is
//        operating in image mode, this field MUST be ignored. If the codec is operating in video mode,
//        this field SHOULD be ignored.
//    numRegions (2 bytes): A 16-bit signed integer. Specifies the number of TS_RFX_REGION
//        (section 2.2.2.3.3) messages following this TS_RFX_FRAME_BEGIN message. That is, the
//        number of regions in the frame.
//

void TS_RFX_FRAME_BEGIN::recv(InStream & stream) {
    TS_RFX_CODEC_CHANNELT::recv(stream);

    ::check_throw(stream, 6, "TS_RFX_FRAME_BEGIN::recv TS_RFX_FRAME_BEGIN", ERR_MCS_PDU_TRUNCATED);

    this->frameIdx = stream.in_uint32_le();
    this->numRegions = stream.in_uint16_le();
}

void TS_RFX_FRAME_BEGIN::send(OutStream & stream) const
{
    TS_RFX_CODEC_CHANNELT::send(stream);

    stream.out_uint32_le(this->frameIdx);
    stream.out_uint16_le(this->numRegions);
}


// 2.2.2.3.3 TS_RFX_REGION
//
// The TS_RFX_REGION message contains information about the list of change rectangles on the
// screen for a specific channel. It also specifies the number of trailing TS_RFX_TILESET (section
// 2.2.2.3.4) messages.
//
//    regionFlags (1 byte): An 8-bit, unsigned integer. Contains a collection of bit-packed property
//        fields. The format of this field is described by the following bitfield diagram.
//  +------------------+
//    | 0 |1 2 3 4 5 6 7 |
//  +------------------+
//     |lrf|   reserved   |
//  +------------------+
//        lrf (1 bit): A 1-bit unsigned integer. This field MUST be set to 0x1. The decoder SHOULD
//            ignore this field.
//        reserved (7 bits: A 7-bit integer reserved for future use. This field MUST be ignored.
//
//    numRects (2 bytes): A 16-bit, unsigned integer. Specifies the number of TS_RFX_RECT
//        (section 2.2.2.1.6) structures present in the rects field.
//    rects (variable): A variable-length array of TS_RFX_RECT (section 2.2.2.1.6) structures. This
//        array defines the region. The number of rectangles in the array is specified in the numRects
//        field. Processing rules for the rectangles in this array are specified in section 3.1.8.2.6.
//    regionType (2 bytes): A 16-bit, unsigned integer. Specifies the region type. This field MUST be
//        set to CBT_REGION (0xCAC1).
//    numTilesets (2 bytes): A 16-bit, unsigned integer. Specifies the number of TS_RFX_TILESET
//        (section 2.2.2.3.4) messages following this TS_RFX_REGION message. This field MUST be set
//        to 0x0001.
//

TS_RFX_REGION::~TS_RFX_REGION() {
    delete [] this->rects;
}

void TS_RFX_REGION::recv(InStream & stream) {
    TS_RFX_CODEC_CHANNELT::recv(stream);

     ::check_throw(stream, 3, "TS_RFX_REGION::recv invalid TS_RFX_REGION", ERR_MCS_PDU_TRUNCATED);

    this->regionFlags = stream.in_uint8();
    if ((this->regionFlags & 0x1) != 0x1) {
        LOG(LOG_ERR, "lrf in_RFX_REGION must be 0x1");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    this->numRects = stream.in_uint16_le();
    if (this->numRects) {
        this->rects = new TS_RFX_RECT[this->numRects]();
        for(int i = 0; i < this->numRects; i++) {
            this->rects[i].recv(stream);
        }
    }

    ::check_throw(stream, 4, "TS_RFX_REGION::recv invalid TS_RFX_REGION", ERR_MCS_PDU_TRUNCATED);

    this->regionType = stream.in_uint16_le();
    if(this->regionType != CBT_REGION) {
        LOG(LOG_ERR, "invalid regionType in TS_RFX_REGION");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    this->numTilesets = stream.in_uint16_le();
    if(this->numTilesets != 1) {
        LOG(LOG_ERR, "expecting numTilesets=1 in TS_RFX_REGION");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }
}

void TS_RFX_REGION::send(OutStream & /*stream*/) const
{
    /* TODO: implement me */
}


// 2.2.2.1.5 TS_RFX_CODEC_QUANT
// The TS_RFX_CODEC_QUANT structure holds the scalar quantization values for the ten sub-bands
// in the 3-level DWT decomposition. Each field in this structure MUST have a value in the range of 6
// to 15.
//
//  +-------+-------+-------+-------+-------+-------+-------+-------+
//  |       |       |    1 1|1 1 1 1|1 1 1 1|2 2 2 2|2 2 2 2|2 2 3 3|
//  |0 1 2 3|4 5 6 7|8 9 0 1|2 3 4 5|6 7 8 9|0 1 2 3|4 5 6 7|8 9 0 1|
//  +-------+-------+-------+-------+-------+-------+-------+-------+
//  |  ll3  |  lh3  |  hl3  |  hh3  |  lh2  |  hl2  |  hh2  |  lh1  |
//  +-------+-------+-------+-------+-------+-------+-------+-------+
//  |  hl1  |  hh1  |
//  +-------+-------+
//
//    LL3 (4 bits): A 4-bit, unsigned integer. The LL quantization factor for the level-3 DWT sub-
//        band.
//    LH3 (4 bits): A 4-bit, unsigned integer. The LH quantization factor for the level-3 DWT sub-
//        band.
//    HL3 (4 bits): A 4-bit, unsigned integer. The HL quantization factors for the level-3 DWT sub-
//        band.
//    HH3 (4 bits): A 4-bit, unsigned integer. The HH quantization factors for the level-3 DWT sub-
//        band.
//    LH2 (4 bits): A 4-bit, unsigned integer. The LH quantization factor for the level-2 DWT sub-
//        band.
//    HL2 (4 bits): A 4-bit, unsigned integer. The HL quantization factor for the level-2 DWT sub-
//        band.
//    HH2 (4 bits): A 4-bit, unsigned integer. The HH quantization factor for the level-2 DWT sub-
//        band.
//    LH1 (4 bits): A 4-bit, unsigned integer. The LH quantization factors for the level-1 DWT sub-
//        band.
//    HL1 (4 bits): A 4-bit, unsigned integer. The HL quantization factors for the level-1 DWT sub-
//        band.
//    HH1 (4 bits): A 4-bit, unsigned integer. The HH quantization factor for the level-1 DWT sub-
//        band.
//

void TS_RFX_CODEC_QUANT::recv(InStream & stream) {
    uint32_t val;

    ::check_throw(stream, 5, "TS_RFX_CODEC_QUANT::recv invalid TS_RFX_CODEC_QUANT", ERR_MCS_PDU_TRUNCATED);

    val = stream.in_uint32_le();
    this->ll3 = val & 0xF;  val >>= 4;
    this->lh3 = val & 0xF;  val >>= 4;
    this->hl3 = val & 0xF;  val >>= 4;
    this->hh3 = val & 0xF;  val >>= 4;
    this->lh2 = val & 0xF;  val >>= 4;
    this->hl2 = val & 0xF;  val >>= 4;
    this->hh2 = val & 0xF;  val >>= 4;
    this->lh1 = val & 0xF;

    val = stream.in_uint8();
    this->hl1 = val & 0xF;  val >>= 4;
    this->hh1 = val & 0xF;
}

void TS_RFX_CODEC_QUANT::send(OutStream & /*stream*/) const
{
    /* TODO: implement me */
}

//
// 2.2.2.3.4.1 TS_RFX_TILE
// The TS_RFX_TILE structure specifies the position of the tile on the frame and contains the encoded
// data for the three tile components of Y, Cb, and Cr.
//
//    quantIdxY (1 byte): An 8-bit, unsigned integer. Specifies an index into the
//        TS_RFX_CODEC_QUANT array provided in the TS_RFX_TILESET message. The specified
//        TS_RFX_CODEC_QUANT element MUST be used for de-quantization of the sub-bands for
//        the Y-component.
//    quantIdxCb (1 byte): An 8-bit, unsigned integer. Specifies an index into the
//        TS_RFX_CODEC_QUANT array provided in the TS_RFX_TILESET message. The specified
//        TS_RFX_CODEC_QUANT element MUST be used for de-quantization of the sub-bands for
//        the Cb-component.
//    quantIdxCr (1 byte): An 8-bit, unsigned integer. Specifies an index into the
//        TS_RFX_CODEC_QUANT array provided in the TS_RFX_TILESET message. The specified
//        TS_RFX_CODEC_QUANT element MUST be used for de-quantization of the sub-bands for
//        the Cr-component.
//    xIdx (2 bytes): A 16-bit, unsigned integer. The X-index of the encoded tile in the screen tile
//        grid.
//    yIdx (2 bytes): A 16-bit, unsigned integer. The Y-index of the encoded tile in the screen tile
//        grid.
//

void TS_RFX_TILE::recv(InStream & stream) {

    ::check_throw(stream, 13, "TS_RFX_TILE::recv invalid TS_RFX_TILE", ERR_MCS_PDU_TRUNCATED);

    uint16_t blockType = stream.in_uint16_le();
    if (blockType != CBT_TILE) {
        LOG(LOG_ERR, "invalid blockType for a TS_RFX_TILE");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    uint16_t blockLen = stream.in_uint32_le();
    if (blockLen < 6 || !stream.in_check_rem(blockLen-6)) {
        LOG(LOG_ERR, "invalid TS_RFX_TILE, need=%u have=%zu", blockLen-4u, stream.in_remain());
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    InStream tileStream({stream.get_current(), blockLen - 6u});

    this->quantIdxY = tileStream.in_uint8();
    this->quantIdxCb = tileStream.in_uint8();
    this->quantIdxCr = tileStream.in_uint8();
    this->xIdx = tileStream.in_uint16_le();
    this->yIdx = tileStream.in_uint16_le();
    this->YLen = tileStream.in_uint16_le();
    this->CbLen = tileStream.in_uint16_le();
    this->CrLen = tileStream.in_uint16_le();

    ::check_throw(stream, this->YLen + this->CbLen + this->CrLen, "TS_RFX_TILE::recv invalid TS_RFX_TILE", ERR_MCS_PDU_TRUNCATED);

    this->YData = new uint8_t[this->YLen]();
    tileStream.in_copy_bytes(this->YData, this->YLen);

    this->CbData = new uint8_t[this->CbLen]();
    tileStream.in_copy_bytes(this->CbData, this->CbLen);

    this->CrData = new uint8_t[this->CrLen]();
    tileStream.in_copy_bytes(this->CrData, this->CrLen);

    stream.in_skip_bytes(blockLen - 6);
}

void TS_RFX_TILE::draw(const RDPSetSurfaceCommand &/*cmd*/, const TS_RFX_TILESET &tileset, RDPSurfaceContent & content)
{
    Primitives::prim_size_t roi_64x64 = { 64, 64 };

    int16_t y_r_buffer[4096+16+8];
    int16_t cb_g_buffer[4096+16+8];
    int16_t cr_b_buffer[4096+16+8];
    const int16_t *yCbCr_channels[3] = { y_r_buffer, cb_g_buffer, cr_b_buffer };

    decodeComponent(tileset.quantVals[this->quantIdxY], tileset.rlgrMode, this->YData, this->YLen, y_r_buffer);
    decodeComponent(tileset.quantVals[this->quantIdxCb], tileset.rlgrMode, this->CbData, this->CbLen, cb_g_buffer);
    decodeComponent(tileset.quantVals[this->quantIdxCr], tileset.rlgrMode, this->CrData, this->CrLen, cr_b_buffer);

    uint8_t *dest = content.data + (this->xIdx * 64) * 4 + (this->yIdx * content.stride * 64);
    Primitives *prims = Primitives::instance();
    Primitives::pstatus_t status = prims->yCbCrToRGB_16s8u_P3AC4R(yCbCr_channels, 64 * sizeof(int16_t),
            dest, content.stride,
            Primitives::PIXEL_FORMAT_BGRX32, &roi_64x64);

    if (status != Primitives::SUCCESS) {
    }
}


void TS_RFX_TILE::decodeComponent(const TS_RFX_CODEC_QUANT &quant, Rlgr::RlgrMode mode, uint8_t *data, size_t len, int16_t *output) {
    int16_t dwt_buffer[4096];

    Rlgr::decode(mode, data, len, output, 4096);
    differential_decode(output + 4032, 64);
    quantization_decode(output, quant);
    Dwt::decode_2d(output, dwt_buffer);
}

void TS_RFX_TILE::differential_decode(int16_t *buffer, size_t size) {
    const int16_t *end = buffer + (size - 1);

    while(buffer != end) {
        buffer[1] += buffer[0];
        buffer++;
    }
}

void TS_RFX_TILE::rfx_quantization_decode_block(const Primitives *prims, int16_t * buffer, int buffer_size, uint32_t factor)
{
    if (factor == 0){
        return;
    }

    prims->lShiftC_16s(buffer, factor, buffer, buffer_size);
}


void TS_RFX_TILE::quantization_decode(int16_t *buffer, const TS_RFX_CODEC_QUANT &quants) {
    const Primitives* prims = Primitives::instance();

    rfx_quantization_decode_block(prims, &buffer[0], 1024, quants.hl1 - 1); /* HL1 */
    rfx_quantization_decode_block(prims, &buffer[1024], 1024, quants.lh1 - 1); /* LH1 */
    rfx_quantization_decode_block(prims, &buffer[2048], 1024, quants.hh1 - 1); /* HH1 */

    rfx_quantization_decode_block(prims, &buffer[3072], 256, quants.hl2 - 1); /* HL2 */
    rfx_quantization_decode_block(prims, &buffer[3328], 256, quants.lh2 - 1); /* LH2 */
    rfx_quantization_decode_block(prims, &buffer[3584], 256, quants.hh2 - 1); /* HH2 */

    rfx_quantization_decode_block(prims, &buffer[3840], 64, quants.hl3 - 1); /* HL3 */
    rfx_quantization_decode_block(prims, &buffer[3904], 64, quants.lh3 - 1); /* LH3 */
    rfx_quantization_decode_block(prims, &buffer[3968], 64, quants.hh3 - 1); /* HH3 */
    rfx_quantization_decode_block(prims, &buffer[4032], 64, quants.ll3 - 1); /* LL3 */
}



TS_RFX_TILESET::~TS_RFX_TILESET() {
    delete [] this->quantVals;
    delete [] this->tiles;
}

static size_t roundTo(size_t l, size_t mod) {
    return ((l + mod - 1) / mod) * mod;
}

// 2.2.2.3.4 TS_RFX_TILESET
// The TS_RFX_TILESET message contains encoding parameters and data for an arbitrary number of
// encoded tiles.
//
//    subtype (2 bytes): A 16-bit, unsigned integer. Specifies the message type. This field MUST be
//        set to CBT_TILESET (0xCAC2).
//    idx (2 bytes): A 16-bit, unsigned integer. Specifies the identifier of the TS_RFX_CONTEXT
//        (section 2.2.2.2.4) message referenced by this TileSet message. This field MUST be set to
//        0x0000. The decoder SHOULD ignore this field.
//    properties (2 bytes): A 16-bit unsigned integer. Contains a collection of bit-packed property
//    fields. The format of this field is described by the following bitmask diagram.
//        +--+------------------+-------+---+
//        |  |     |   |       |1 1 1 1|1 1|
//        | 0|1 2 3|4 5|6 7 8 9|0 1 2 3|4 5|
//        +--+-----+---+-------+-------+---+
//        |lt|flags|cct|  xft  |   et  |qt |
//        +--+-----+---+-------+-------+---+
//
//            lt (1 bit): A 1-bit field that specifies whether this is the last TS_RFX_TILESET in the
//                region. This field MUST be set to TRUE (0x1). The decoder SHOULD ignore this field.
//            flags (3 bits): A 3-bit unsigned integer. Specifies operational flags.
//                CODEC_MODE - 0x02 The codec is operating in image mode. If this flag is not set, the codec is
//                        operating in video mode.
//                The encoder MUST set this value to the value of flags that is set in the properties field of
//                TS_RFX_CONTEXT. The decoder MUST ignore this flag and MUST use the flags specified
//                in the flags field of the TS_RFX_CONTEXT.
//            cct (2 bits): A 2-bit unsigned integer. Specifies the color conversion transform. This field
//                MUST be set to COL_CONV_ICT (0x1) to specify the transform defined by the equations
//                in sections 3.1.8.1.3 and 3.1.8.2.5. The decoder SHOULD ignore this field.
//            xft (4 bits): A 4-bit unsigned integer. Specifies the DWT. This field MUST be set to
//                CLW_XFORM_DWT_53_A (0x1), which indicates the DWT given by the equations in
//                sections 3.1.8.1.4 and 3.1.8.2.4. The decoder SHOULD ignore this field.
//            et (4 bits): A 4-bit unsigned integer. Specifies the entropy algorithm. This field MUST be
//                set to one of the following values.
//                    CLW_ENTROPY_RLGR1 - 0x01 RLGR algorithm as detailed in 3.1.8.1.7.1.
//                    CLW_ENTROPY_RLGR3 - 0x04 RLGR algorithm as detailed in 3.1.8.1.7.2.
//            qt (2 bits): A 2-bit unsigned integer. Specifies the quantization type. This field
//    numQuant (1 byte): An 8-bit, unsigned integer. Specifies the number of
//        TS_RFX_CODEC_QUANT (section 2.2.2.1.5) structures present in the quantVals field.
//    tileSize (1 byte): An 8-bit, unsigned integer. Specifies the width and height of a tile. This field
//        MUST be set to 0x40. The decoder SHOULD ignore this field.
//    numTiles (2 bytes): A 16-bit, unsigned integer. Specifies the number of TS_RFX_TILE (section
//        2.2.2.3.4.1) structures present in the tiles field.
//    tilesDataSize (4 bytes): A 32-bit, unsigned integer. Specifies the size, in bytes, of the tiles
//        field. The tiles field contains encoded data for all of the tiles that have changed.
//    quantVals (variable): A variable-length array of TS_RFX_CODEC_QUANT (section 2.2.2.1.5)
//        structures. The number of structures present in the array is indicated in the numQuant field.
//    tiles (variable): A variable-length array of TS_RFX_TILE (section 2.2.2.3.4.1) structures. The
//        number of structures present in the array is indicated in the numTiles field, while the total
//    size, in bytes, of this field is specified by the tilesDataSize field.

void TS_RFX_TILESET::recv(InStream & stream, const RDPSetSurfaceCommand &cmd, const SubRegion &region, gdi::GraphicApi & drawable) {
    TS_RFX_CODEC_CHANNELT::recv(stream);

    ::check_throw(stream, 12, "TS_RFX_TILESET::recv invalid TS_RFX_TILESET", ERR_MCS_PDU_TRUNCATED);

    const uint8_t *tilesetStream = stream.get_current();

    this->subType = stream.in_uint16_le();
    if (this->subType != CBT_TILESET) {
        LOG(LOG_ERR, "expecting subType=CBT_TILESET in TS_RFX_TILESET");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    this->idx = stream.in_uint16_le();
    if (this->idx != 0x0000) {
        LOG(LOG_ERR, "expecting idx=0 in TS_RFX_TILESET");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    this->properties = stream.in_uint16_le();
    uint8_t mode = (properties >> 10) & 0xf;
    switch(mode) {
    case 0x01:
        this->rlgrMode = Rlgr::RLGR1;
        break;
    case 0x04:
        this->rlgrMode = Rlgr::RLGR3;
        break;
    default:
        LOG(LOG_ERR, "unhandled mode %d in TS_RFX_TILESET", mode);
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    this->numQuant = stream.in_uint8();
    if (this->numQuant){
        quantVals = new TS_RFX_CODEC_QUANT[numQuant]();
    }

    this->tileSize = stream.in_uint8();
    if (this->tileSize != 0x40) {
        LOG(LOG_ERR, "tileSize idx=0x40 in TS_RFX_TILESET");
        throw Error(ERR_MCS_PDU_TRUNCATED);
    }

    this->numTiles = stream.in_uint16_le();
    if (numTiles){
        tiles = new TS_RFX_TILE[numTiles]();
    }

    this->tileDataSize = stream.in_uint32_le();

    for (int i = 0; i < this->numQuant; i++){
        this->quantVals[i].recv(stream);
    }

    ::check_throw(stream, tileDataSize, "TS_RFX_TILESET::recv invalid TS_RFX_TILESET tileDataSize", ERR_MCS_PDU_TRUNCATED);

    InStream tilesStream({stream.get_current(), this->tileDataSize});
    for (int i = 0; i < this->numTiles; i++){
        this->tiles[i].recv(tilesStream);
    }

    uint16_t width = roundTo(cmd.width, 64);
    uint16_t height = roundTo(cmd.height, 64);
    RDPSurfaceContent content(width, height, width * 4, cmd.destRect, region,
            u8_array_view(tilesetStream, 12 + this->numQuant * 5 + this->tileDataSize));
    for (int i = 0; i < numTiles; i++){
        this->tiles[i].draw(cmd, *this, content);
    }

    drawable.draw(cmd, content);
}



void RfxDecoder::recv(InStream & stream, const RDPSetSurfaceCommand & cmd, gdi::GraphicApi & drawable) {
    while (stream.in_check_rem(6)) {

        ::check_throw(stream, 6, "RfxDecoder::recv invalid RfxDecoder", ERR_MCS_PDU_TRUNCATED);

        uint16_t blockType = stream.in_uint16_le();
        uint32_t blockLen = stream.in_uint32_le();

        if (blockLen < 6) {
            LOG(LOG_ERR, "Invalid blockLen=%u in RFX_PACKET", blockLen);
            throw Error(ERR_MCS_PDU_TRUNCATED);
        }

        ::check_throw(stream, blockLen-6, "RFX_PACKET::recv invalid RFX_PACKET", ERR_MCS_PDU_TRUNCATED);

        InStream packetStream({stream.get_current(), blockLen - 6});
        stream.in_skip_bytes(blockLen - 6);

        switch(this->decoderState) {
        // ####################################################################
        case RFX_WAITING_SYNC: {
            switch(blockType) {
            case WBT_SYNC: {
                TS_RFX_SYNC sync;
                sync.recv(packetStream);
                this->decoderState = RFX_WAITING_PROPERTIES;
                break;
            }
            default:
                LOG(LOG_ERR, "Expecting a remoteFx Sync packet");
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }
            break;
        }

        // ####################################################################
        case RFX_WAITING_PROPERTIES: {
            switch(blockType) {
            case WBT_CONTEXT: {
                TS_RFX_CONTEXT rfxContext;
                rfxContext.recv(packetStream);
                haveFlags |= FLAG_CONTEXT;
                break;
            }
            case WBT_CODEC_VERSION: {
                TS_RFX_CODEC_VERSIONS rfxVersions;
                rfxVersions.recv(packetStream);
                haveFlags |= FLAG_VERSIONS;
                break;
            }
            case WBT_CHANNELS: {
                TS_RFX_CHANNELS rfxChannels;
                rfxChannels.recv(packetStream);
                haveFlags |= FLAG_CHANNELS;
                break;
            }
            default:
                LOG(LOG_ERR, "Not expecting a blockType=%u in state RFX_WAITING_PROPERTIES", blockType);
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            if (haveFlags == FLAG_HAVE_ALL){
                this->decoderState = RFX_WAITING_FRAME;
            }
            break;
        }

        // ####################################################################
        case RFX_WAITING_FRAME: {
            switch(blockType) {
            case WBT_CONTEXT:
            case WBT_CODEC_VERSION:
            case WBT_CHANNELS:
                break;
            case WBT_FRAME_BEGIN: {
                TS_RFX_FRAME_BEGIN frameBegin;
                frameBegin.recv(packetStream);
                break;
            }
            case WBT_FRAME_END: {
                TS_RFX_FRAME_END frameEnd;
                frameEnd.recv(packetStream);
                break;
            }
            case WBT_REGION: {
                this->currentRegion.recv(packetStream);
                break;
            }
            case WBT_EXTENSION: {
                TS_RFX_TILESET tileset;
                SubRegion region;
                for (int i = 0; i < this->currentRegion.numRects; i++) {
                    const TS_RFX_RECT &rfxRect = this->currentRegion.rects[i];
                    region.add_rect( Rect(rfxRect.x, rfxRect.y, rfxRect.width, rfxRect.height) );
                }
                tileset.recv(packetStream, cmd, region, drawable);
                break;
            }
            default:
                LOG(LOG_ERR, "Not expecting a blockType=%u in state RFX_WAITING_FRAME", blockType);
                break;
            }
            break;
        }
        }
    }
}


RfxEncoder::RfxEncoder()
    : initialized(false)
    , frameCounter(0)
{

}

void RfxEncoder::sendInitSequence() {
    StaticOutStream<2000> initSeq;

    TS_RFX_SYNC sync;
    sync.send(initSeq);

    TS_RFX_CONTEXT context;
    context.send(initSeq);

    TS_RFX_CODEC_VERSIONS versions;
    versions.send(initSeq);

    TS_RFX_CHANNELS channels;
    channels.setChannel(0, 0, 0);
    channels.send(initSeq);
}

void RfxEncoder::sendFrame() {
    if (!this->initialized) {
        this->sendInitSequence();
        this->initialized = true;
    }

    TS_RFX_FRAME_BEGIN frameBegin;
    frameBegin.frameIdx = ++this->frameCounter;
    // frameBegin.send()


    [[maybe_unused]]
    TS_RFX_FRAME_END frameEnd;
    //frameEnd.send()
}

