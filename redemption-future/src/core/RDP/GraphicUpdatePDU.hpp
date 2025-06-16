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
   Copyright (C) Wallix 2011
   Author(s): Christophe Grosjean, Raphael Zhou
*/


#pragma once

#include <cinttypes>

#include "utils/log.hpp"
#include "core/RDP/RDPSerializer.hpp"
#include "core/RDP/gcc/userdata/mcs_channels.hpp"
#include "core/RDP/sec.hpp"
#include "core/RDP/mcs.hpp"
#include "core/RDP/x224.hpp"

static inline void send_data_indication_ex( Transport & trans
                                          , int encryptionLevel, CryptContext & encrypt
                                          , uint16_t initiator
                                          , OutReservedStreamHelper & stream)
{
    StaticOutStream<256> security_header;
    SEC::Sec_Send sec(security_header, stream.get_packet(), 0, encrypt, encryptionLevel);
    stream.copy_to_head(security_header.get_produced_bytes());

    StaticOutStream<256> mcs_header;
    MCS::SendDataIndication_Send mcs( mcs_header
                                    , initiator
                                    , GCC::MCS_GLOBAL_CHANNEL
                                    , 1 // dataPriority
                                    , 3 // segmentation
                                    , stream.get_packet().size()
                                    , MCS::PER_ENCODING);

    StaticOutStream<256> x224_header;
    X224::DT_TPDU_Send(x224_header, stream.get_packet().size() + mcs_header.get_offset());

    auto packet = stream.copy_to_head(x224_header.get_produced_bytes(), mcs_header.get_produced_bytes());
    trans.send(packet);
}

template<class... DataWriter>
void send_data_indication_ex( Transport & trans
                            , uint16_t channelId
                            , int encryptionLevel, CryptContext & encrypt
                            , uint16_t initiator, DataWriter... data_writer)
{
    write_packets(
        trans,
        data_writer...,
        [&](StreamSize<256> /*maxlen*/, OutStream & security_header, writable_bytes_view packet) {
            SEC::Sec_Send sec(security_header, packet, 0, encrypt, encryptionLevel);
        },
        [&](StreamSize<256> /*maxlen*/, OutStream & mcs_header, std::size_t packet_size) {
            MCS::SendDataIndication_Send mcs( mcs_header
                                            , initiator
                                            , channelId
                                            , 1 // dataPriority
                                            , 3 // segmentation
                                            , packet_size
                                            , MCS::PER_ENCODING);
        },
        [](StreamSize<256> /*maxlen*/, OutStream & x224_header, std::size_t packet_size) {
            X224::DT_TPDU_Send(x224_header, packet_size);
        }
    );
}

inline
void send_share_data_ex( Transport & trans, uint8_t pduType2, bool compression_support
                       , rdp_mppc_enc * mppc_enc, uint32_t shareId, int encryptionLevel
                       , CryptContext & encrypt, uint16_t initiator
                       , OutReservedStreamHelper & data, bool verbose)
{
    assert(!compression_support || mppc_enc);

    StaticOutReservedStreamHelper<1024, 65536-1024> data_compressed;
    OutReservedStreamHelper *data_ = &data;

    uint8_t compressionFlags = 0;

    if (compression_support) {
        uint16_t compressed_data_size = 0;

        mppc_enc->compress( data.get_packet().data()
                          , data.get_packet().size()
                          , compressionFlags, compressed_data_size
                          , rdp_mppc_enc::MAX_COMPRESSED_DATA_SIZE_UNUSED
                          );

        if (compressionFlags & PACKET_COMPRESSED) {
            mppc_enc->get_compressed_data(data_compressed.get_data_stream());
            data_ = &data_compressed;
        }
    }


    StaticOutStream<256> share_data_header;
    ShareData share_data(share_data_header);
    share_data.emit_begin( pduType2, shareId, RDP::STREAM_MED
                         , data.get_packet().size() + 18 /* TS_SHAREDATAHEADER(18) */
                         , compressionFlags
                         , (compressionFlags ? data_->get_packet().size() + 18 /* TS_SHAREDATAHEADER(18) */ : 0)
                         );
    share_data.emit_end();
    data_->copy_to_head(share_data_header.get_produced_bytes());

    StaticOutStream<256> share_ctrl_header;
    ShareControl_Send( share_ctrl_header, PDUTYPE_DATAPDU, initiator + GCC::MCS_USERCHANNEL_BASE
                     , data_->get_packet().size());
    data_->copy_to_head(share_ctrl_header.get_produced_bytes());

    if (verbose) {
        LOG(LOG_INFO, "Sec clear payload to send:");
        hexdump_d(data_->get_packet());
    }

    ::send_data_indication_ex(trans, encryptionLevel, encrypt, initiator, *data_);
}

inline void send_fast_path_data(
    Transport & trans, bool compression_support,
    rdp_mppc_enc * mppc_enc, int encryptionLevel,
    CryptContext & encrypt, FastPath::UpdateType updateCode,
    OutReservedStreamHelper & data_common)
{
    assert(!compression_support || mppc_enc);

    uint8_t compression = 0;
    uint32_t fragmentId;
    uint32_t fragmentMax = 0x3fff - 20; /* taken in FreeRDP */
    uint32_t startAt = 0;

    if (compression_support) {
        fragmentMax = std::min(fragmentMax, mppc_enc->get_max_data_block_size() - 20);
    }

    fragmentId = 0;

    bytes_view fullPacket = data_common.get_packet();
    uint32_t remaining = checked_int{fullPacket.size()};
    uint32_t payloadSize = remaining;
    do {
        uint8_t compressionFlags = 0;
        uint32_t fragmentSize = (remaining > fragmentMax) ? fragmentMax : remaining;
        OutReservedStreamHelper packetFragment = data_common.get_sub_stream(startAt, fragmentSize);

        StaticOutReservedStreamHelper<1024, 65536-1024> data_common_compressed;
        OutReservedStreamHelper * fragmentPayload = &packetFragment;

        if (compression_support) {
            uint16_t compressed_data_size;

            mppc_enc->compress( packetFragment.get_data_stream().get_data()
                              , checked_int{fragmentSize}
                              , compressionFlags, compressed_data_size
                              , rdp_mppc_enc::MAX_COMPRESSED_DATA_SIZE_UNUSED
                              );

            if (compressionFlags & PACKET_COMPRESSED) {
                compression = FastPath::FASTPATH_OUTPUT_COMPRESSION_USED;

                mppc_enc->get_compressed_data(data_common_compressed.get_data_stream());
                fragmentPayload = &data_common_compressed;
            }
        }

        startAt += fragmentSize;

        StaticOutStream<8> update_header;
        uint8_t fragFlags = (fragmentId != 0)
            ? ((startAt == payloadSize)
                ? FastPath::FASTPATH_FRAGMENT_LAST
                : FastPath::FASTPATH_FRAGMENT_NEXT)
            : ((startAt == payloadSize)
                ? FastPath::FASTPATH_FRAGMENT_SINGLE
                : FastPath::FASTPATH_FRAGMENT_FIRST);

        // Fast-Path Update (TS_FP_UPDATE)
        FastPath::Update_Send Upd( update_header
                                 , checked_int{fragmentPayload->get_packet().size()}
                                 , static_cast<uint8_t>(updateCode)
                                 , fragFlags
                                 , compression
                                 , compressionFlags
                                 );
        fragmentPayload->copy_to_head(update_header.get_produced_bytes());

        StaticOutStream<256> server_update_header;
        // Server Fast-Path Update PDU (TS_FP_UPDATE_PDU)
        FastPath::ServerUpdatePDU_Send SvrUpdPDU( server_update_header
                                                , fragmentPayload->get_packet().data()
                                                , fragmentPayload->get_packet().size()
                                                , (encryptionLevel > 1)
                                                    ? FastPath::FASTPATH_OUTPUT_ENCRYPTED
                                                    : 0
                                                , encrypt
                                                );
        fragmentPayload->copy_to_head(server_update_header.get_produced_bytes());

        trans.send(fragmentPayload->get_packet());

        fragmentId++;
        remaining -= fragmentSize;
    } while(remaining);
}

inline
void send_server_update(
    FastPath::UpdateType updateCode, uint8_t pduType2,
    Transport & trans, bool fastpath_support, bool compression_support,
    rdp_mppc_enc * mppc_enc, uint32_t shareId, int encryptionLevel,
    CryptContext & encrypt, uint16_t initiator,
    OutReservedStreamHelper & data_common, bool verbose)
{
    if (fastpath_support) {
        send_fast_path_data(
            trans, compression_support, mppc_enc, encryptionLevel,
            encrypt, updateCode, data_common);
    }
    else {
        send_share_data_ex(
            trans, pduType2, compression_support, mppc_enc, shareId,
            encryptionLevel, encrypt, initiator, data_common, verbose);
    }
}



// MS-RDPEGDI 2.2.2.2 Fast-Path Orders Update (TS_FP_UPDATE_ORDERS)
// ================================================================
// The TS_FP_UPDATE_ORDERS structure contains primary, secondary, and alternate
// secondary drawing orders aligned on byte boundaries. This structure conforms
// to the layout of a Fast-Path Update (see [MS-RDPBCGR] section 2.2.9.1.2.1)
// and is encapsulated within a Fast-Path Update PDU (see [MS-RDPBCGR] section
// 2.2.9.1.2.1.1).

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field
//   is the same as the updateHeader byte field described in the Fast-Path
//   Update structure (see [MS-RDPBCGR] section 2.2.9.1.2.1). The updateCode
//   bitfield (4 bits in size) MUST be set to FASTPATH_UPDATETYPE_ORDERS (0x0).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this
//   optional field (as well as the possible values) is the same as the
//   compressionFlags field described in the Fast-Path Update structure
//   specified in [MS-RDPBCGR] section 2.2.9.1.2.1.

// size (2 bytes): A 16-bit, unsigned integer. The format of this field (as well
//   as the possible values) is the same as the size field described in the
//   Fast-Path Update structure specified in [MS-RDPBCGR] section 2.2.9.1.2.1.

// numberOrders (2 bytes): A 16-bit, unsigned integer. The number of Drawing
//   Order (section 2.2.2.1.1) structures contained in the orderData field.

// orderData (variable): A variable-sized array of Drawing Order (section
//   2.2.2.1.1) structures packed on byte boundaries. Each structure contains a
//   primary, secondary, or alternate secondary drawing order. The controlFlags
//   field of the Drawing Order identifies the type of drawing order.


// MS-RDPEGDI 2.2.2.1 Orders Update (TS_UPDATE_ORDERS_PDU_DATA)
// ============================================================
// The TS_UPDATE_ORDERS_PDU_DATA structure contains primary, secondary, and
// alternate secondary drawing orders aligned on byte boundaries. This structure
// conforms to the layout of a Slow Path Graphics Update (see [MS-RDPBCGR]
// section 2.2.9.1.1.3.1) and is encapsulated within a Graphics Update PDU (see
// [MS-RDPBCGR] section 2.2.9.1.1.3.1.1).

// shareDataHeader (18 bytes): Share Data Header (see [MS-RDPBCGR], section
//   2.2.8.1.1.1.2) containing information about the packet. The type subfield
//   of the pduType field of the Share Control Header (section 2.2.8.1.1.1.1)
//   MUST be set to PDUTYPE_DATAPDU (7). The pduType2 field of the Share Data
//   Header MUST be set to PDUTYPE2_UPDATE (2).

// updateType (2 bytes): A 16-bit, unsigned integer. The field contains the
//   graphics update type. This field MUST be set to UPDATETYPE_ORDERS (0x0000).

// pad2OctetsA (2 bytes): A 16-bit, unsigned integer used as a padding field.
//   Values in this field are arbitrary and MUST be ignored.

// numberOrders (2 bytes): A 16-bit, unsigned integer. The number of Drawing
//   Order (section 2.2.2.1.1) structures contained in the orderData field.

// pad2OctetsB (2 bytes): A 16-bit, unsigned integer used as a padding field.
//   Values in this field are arbitrary and MUST be ignored.

// orderData (variable): A variable-sized array of Drawing Order (section
//   2.2.2.1.1) structures packed on byte boundaries. Each structure contains a
//   primary, secondary, or alternate secondary drawing order. The controlFlags
//   field of the Drawing Order identifies the type of drawing order.

namespace detail
{
    // fix: field 'buffer_stream_orders' is uninitialized
    struct GraphicsUpdatePDUBuffer
    {
        StaticOutReservedStreamHelper<1024, 65536-1024> buffer_stream_orders;
        StaticOutReservedStreamHelper<1024, 65536-1024> buffer_stream_bitmaps;
    };
} // namespace detail

class GraphicsUpdatePDU
: private detail::GraphicsUpdatePDUBuffer
, public RDPSerializer
{
    uint16_t     & userid;
    int          & shareid;
    int          & encryptionLevel;
    CryptContext & encrypt;

    uint32_t offset_bitmap_count;

    bool fastpath_support;

    rdp_mppc_enc * mppc_enc;
    bool           compression;

    Transport & trans;

    PointerCache & pointer_cache;

public:
    GraphicsUpdatePDU( Transport & trans
                     , uint16_t & userid
                     , int & shareid
                     , int & encryptionLevel
                     , CryptContext & encrypt
                     , const BitsPerPixel bpp
                     , BmpCache & bmp_cache
                     , GlyphCache & gly_cache
                     , PointerCache & pointer_cache
                     , const int bitmap_cache_version
                     , const int use_bitmap_comp
                     , const int op2
                     , size_t max_data_block_size
                     , bool experimental_enable_serializer_data_block_size_limit
                     , bool fastpath_support
                     , rdp_mppc_enc * mppc_enc
                     , bool compression
                     , size_t max_request_size
                     , RDPSerializerVerbose verbose
                     )
        : RDPSerializer( this->buffer_stream_orders.get_data_stream()
                       , this->buffer_stream_bitmaps.get_data_stream()
                       , bpp, bmp_cache, gly_cache, bitmap_cache_version
                       , use_bitmap_comp, op2, max_data_block_size
                       , experimental_enable_serializer_data_block_size_limit, max_request_size, verbose)
        , userid(userid)
        , shareid(shareid)
        , encryptionLevel(encryptionLevel)
        , encrypt(encrypt)
        , offset_bitmap_count(0)
        , fastpath_support(fastpath_support)
        , mppc_enc(mppc_enc)
        , compression(compression)
        , trans(trans)
        , pointer_cache(pointer_cache)
    {
        this->init_orders();
        this->init_bitmaps();
    }

    ~GraphicsUpdatePDU() = default;

    void init_orders() {
        LOG_IF(bool(this->verbose & RDPSerializerVerbose::internal_buffer), LOG_INFO
          , "GraphicsUpdatePDU::init::Initializing orders batch mcs_userid=%u shareid=%d"
          , this->userid, this->shareid);
    }

    void init_bitmaps() {
        LOG_IF(bool(this->verbose & RDPSerializerVerbose::internal_buffer), LOG_INFO
          , "GraphicsUpdatePDU::init::Initializing bitmaps batch mcs_userid=%u shareid=%d"
          , this->userid, this->shareid);

        this->stream_bitmaps.out_uint16_le(RDP_UPDATE_BITMAP);  // updateType (2 bytes)
        this->offset_bitmap_count = this->stream_bitmaps.get_offset();
        this->stream_bitmaps.out_clear_bytes(2);  // number of bitmap, set later
    }

public:
    void sync() final
    {
        this->flush_bitmaps();
        this->flush_orders();
    }

protected:
    void flush_orders() final
    {
        if (this->order_count > 0){
            LOG_IF(bool(this->verbose & RDPSerializerVerbose::internal_buffer), LOG_INFO,
                "GraphicsUpdatePDU::flush_orders: order_count=%zu", this->order_count);

            if (this->fastpath_support) {
                StaticOutStream<2> data;
                data.out_uint16_le(this->order_count);
                this->buffer_stream_orders.copy_to_head(data.get_produced_bytes());

                send_fast_path_data(
                    this->trans, this->compression, this->mppc_enc,
                    this->encryptionLevel, this->encrypt,
                    FastPath::UpdateType::ORDERS, this->buffer_stream_orders);
            }
            else {
                StaticOutStream<64> data;

                data.out_uint16_le(RDP_UPDATE_ORDERS);
                data.out_clear_bytes(2);
                data.out_uint16_le(this->order_count);
                data.out_clear_bytes(2);

                this->buffer_stream_orders.copy_to_head(data.get_produced_bytes());

                send_share_data_ex(
                    this->trans, PDUTYPE2_UPDATE, this->compression,
                    this->mppc_enc, this->shareid, this->encryptionLevel,
                    this->encrypt, this->userid, this->buffer_stream_orders,
                    bool(this->verbose & (RDPSerializerVerbose::internal_buffer
                                        | RDPSerializerVerbose::sec_decrypted)));
            }

            this->order_count = 0;
            this->buffer_stream_orders.rewind();
            this->init_orders();
        }
    }

    void flush_bitmaps() final
    {
        if (this->bitmap_count > 0) {
            LOG_IF(bool(this->verbose & RDPSerializerVerbose::internal_buffer), LOG_INFO
              , "GraphicsUpdatePDU::flush_bitmaps: bitmap_count=%zu offset=%" PRIu32
              , this->bitmap_count, this->offset_bitmap_count);

            this->stream_bitmaps
                .stream_at(this->offset_bitmap_count)
                .out_uint16_le(this->bitmap_count);

            send_server_update(
                FastPath::UpdateType::BITMAP, PDUTYPE2_UPDATE,
                this->trans, this->fastpath_support, this->compression,
                this->mppc_enc, this->shareid, this->encryptionLevel, this->encrypt,
                this->userid, this->buffer_stream_bitmaps,
                bool(this->verbose & (RDPSerializerVerbose::internal_buffer
                                    | RDPSerializerVerbose::sec_decrypted)));

            this->bitmap_count = 0;
            this->buffer_stream_bitmaps.rewind();
            this->init_bitmaps();
        }
    }

private:
// 2.2.9.1.2.1 Fast-Path Update (TS_FP_UPDATE)
// ==========================================

// The TS_FP_UPDATE structure is used to describe and encapsulate the data for a fast-path update sent from server to client.
// All fast-path updates conform to this basic structure (sections 2.2.9.1.2.1.1 to 2.2.9.1.2.1.10).

// updateHeader (1 byte):  An 8-bit, unsigned integer. Three pieces of information are collapsed into this byte:

//    Fast-path update type
//    Fast-path fragment sequencing
//    Compression usage indication

//    The format of the updateHeader byte is described by the following bitmask diagram.

// 2.2.9.1.2.1.1 Fast-Path Palette Update (TS_FP_UPDATE_PALETTE)
// =============================================================


// The TS_FP_UPDATE_PALETTE structure is the fast-path variant of the TS_UPDATE_PALETTE (section 2.2.9.1.1.3.1.1) structure.

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is the same as the updateHeader byte field,
// specified in the Fast-Path Update (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST be set to FASTPATH_UPDATETYPE_PALETTE (2).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values) is
// the same as the compressionFlags field specified in the Fast-Path Update structure.

// size (2 bytes): A 16-bit, unsigned integer. The format of this field (as well as the possible values) is the same as the
// size field specified in the Fast-Path Update structure.

// paletteUpdateData (variable): Variable-length palette data. Both slow-path and fast-path utilize the same data format,
// a Palette Update Data (section 2.2.9.1.1.3.1.1.1) structure, to represent this information.

// 2.2.9.1.2.1.2 Fast-Path Bitmap Update (TS_FP_UPDATE_BITMAP)
// ==========================================================

// The TS_FP_UPDATE_BITMAP structure is the fast-path variant of the TS_UPDATE_BITMAP (section 2.2.9.1.1.3.1.2) structure.

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is the same as the updateHeader byte field
// specified in the Fast-Path Update (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST be set
// to FASTPATH_UPDATETYPE_BITMAP (1).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values)
// is the same as the compressionFlags field specified in the Fast-Path Update structure.

// size (2 bytes): A 16-bit, unsigned integer. The format of this field (as well as the possible values) is the same
// as the size field specified in the Fast-Path Update structure.

// bitmapUpdateData (variable):  Variable-length bitmap data. Both slow-path and fast-path utilize the same data format,
// a Bitmap Update Data (section 2.2.9.1.1.3.1.2.1) structure, to represent this information.

// 2.2.9.1.2.1.3 Fast-Path Synchronize Update (TS_FP_UPDATE_SYNCHRONIZE)
// =====================================================================

// The TS_FP_UPDATE_SYNCHRONIZE structure is the fast-path variant of the TS_UPDATE_SYNCHRONIZE_PDU_DATA
// (section 2.2.9.1.1.3.1.3) structure.

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is the same as the updateHeader byte field
//  described in the Fast-Path Update (section 2.2.9.1.2.1). The updateCode bitfield (4 bits in size) MUST be set
// to FASTPATH_UPDATETYPE_SYNCHRONIZE (3).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values)
// is the same as the compressionFlags field described in the Fast-Path Update structure.

// size (2 bytes): A 16-bit, unsigned integer. This field MUST be set to zero.

// 2.2.9.1.2.1.4 Fast-Path Pointer Position Update (TS_FP_POINTERPOSATTRIBUTE)
// ===========================================================================

// The TS_FP_POINTERPOSATTRIBUTE structure is the fast-path variant of the TS_POINTERPOSATTRIBUTE structure (section 2.2.9.1.1.4.2).

// updateHeader (1 byte): The format of this field is the same as the updateHeader byte field specified in the Fast-Path Update
// (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST be set to FASTPATH_UPDATETYPE_PTR_POSITION (8).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values)
// is the same as the compressionFlags field specified in the Fast-Path Update structure.

// size (2 bytes): A 16-bit, unsigned integer. The format of this field (as well as the possible values) is the same as the
// size field specified in the Fast-Path Update structure.

// pointerPositionUpdateData (4 bytes): Pointer coordinates. Both slow-path and fast-path utilize the same data format,
// a Pointer Position Update (section 2.2.9.1.1.4.2) structure, to represent this information.

// 2.2.9.1.2.1.5 Fast-Path System Pointer Hidden Update (TS_FP_SYSTEMPOINTERHIDDENATTRIBUTE)
// =========================================================================================

// The TS_FP_SYSTEMPOINTERHIDDENATTRIBUTE structure is used to hide the pointer.

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is the same as the updateHeader byte field
//  specified in the Fast-Path Update (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST be
// set to FASTPATH_UPDATETYPE_PTR_NULL (5).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values)
// is the same as the compressionFlags field specified in the Fast-Path Update structure.

// size (2 bytes): A 16-bit, unsigned integer. This field MUST be set to zero.

// 2.2.9.1.2.1.6 Fast-Path System Pointer Default Update (TS_FP_SYSTEMPOINTERDEFAULTATTRIBUTE)
// ===========================================================================================

// The TS_FP_SYSTEMPOINTERDEFAULTATTRIBUTE structure is used to set the shape of the pointer to the operating system default.

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is the same as the updateHeader byte field
// specified in the Fast-Path Update (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST be set
// to FASTPATH_UPDATETYPE_PTR_DEFAULT (6).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values)
// is the same as the compressionFlags field specified in the Fast-Path Update structure.

// size (2 bytes): A 16-bit, unsigned integer. This field MUST be set to zero.

// 2.2.9.1.2.1.7 Fast-Path Color Pointer Update (TS_FP_COLORPOINTERATTRIBUTE)
// =========================================================================

// The TS_FP_COLORPOINTERATTRIBUTE structure is the fast-path variant of the TS_COLORPOINTERATTRIBUTE (section 2.2.9.1.1.4.4)
// structure.

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is the same as the updateHeader byte field
// specified in the Fast-Path Update (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST be set
// to FASTPATH_UPDATETYPE_COLOR (9).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values)
// is the same as the compressionFlags field specified in the Fast-Path Update structure.

// size (2 bytes): A 16-bit, unsigned integer. The format of this field (as well as the possible values) is the same as
// the size field specified in the Fast-Path Update structure.

// colorPointerUpdateData (variable): Color pointer data. Both slow-path and fast-path utilize the same data format,
// a Color Pointer Update (section 2.2.9.1.1.4.4) structure, to represent this information.

// 2.2.9.1.2.1.8 Fast-Path New Pointer Update (TS_FP_POINTERATTRIBUTE)
// ===================================================================

// The TS_FP_POINTERATTRIBUTE structure is the fast-path variant of the TS_POINTERATTRIBUTE (section 2.2.9.1.1.4.5) structure.

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is the same as the updateHeader byte field
// specified in the Fast-Path Update (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST be set to FASTPATH_UPDATETYPE_POINTER (11).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values)
// is the same as the compressionFlags field specified in the Fast-Path Update structure.

// size (2 bytes): A 16-bit, unsigned integer. The format of this field (as well as the possible values) is the same as the size
// field specified in the Fast-Path Update structure.

// newPointerUpdateData (variable): Color pointer data at arbitrary color depth. Both slow-path and fast-path utilize the same
// data format, a New Pointer Update (section 2.2.9.1.1.4.5) structure, to represent this information.

// 2.2.9.1.2.1.9 Fast-Path Cached Pointer Update (TS_FP_CACHEDPOINTERATTRIBUTE)
// ============================================================================

// The TS_FP_CACHEDPOINTERATTRIBUTE structure is the fast-path variant of the TS_CACHEDPOINTERATTRIBUTE (section 2.2.9.1.1.4.6)
// structure.

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is the same as the updateHeader byte field
// specified in the Fast-Path Update (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST be set to FASTPATH_UPDATETYPE_CACHED (10).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values)
// is the same as the compressionFlags field specified in the Fast-Path Update structure.

// size (2 bytes): A 16-bit, unsigned integer. The format of this field (as well as the possible values) is the same as
// the size field specified in the Fast-Path Update structure.

// cachedPointerUpdateData (2 bytes):  Cached pointer data. Both slow-path and fast-path utilize the same data format,
// a Cached Pointer Update (section 2.2.9.1.1.4.6) structure, to represent this information.

// 2.2.9.1.2.1.10 Fast-Path Surface Commands Update (TS_FP_SURFCMDS)
// =================================================================

// The TS_FP_SURFCMDS structure encapsulates one or more Surface Command (section 2.2.9.1.2.1.10.1) structures.

// updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is the same as the updateHeader byte field
// specified in the Fast-Path Update (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST be set
// to FASTPATH_UPDATETYPE_SURFCMDS (4).

// compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional field (as well as the possible values)
// is the same as the compressionFlags field specified in the Fast-Path Update (section 2.2.9.1.2.1) structure.

// size (2 bytes): A 16-bit, unsigned integer. The format of this field (as well as the possible values) is the same as
// the size field specified in the Fast-Path Update structure.

// surfaceCommands (variable): An array of Surface Command (section 2.2.9.1.2.1.10.1) structures containing a collection
// of commands to be processed by the client.


//    2.2.9.1.1.4     Server Pointer Update PDU (TS_POINTER_PDU)
//    ----------------------------------------------------------
//    The Pointer Update PDU is sent from server to client and is used to convey
//    pointer information, including pointers' bitmap images, use of system or
//    hidden pointers, use of cached cursors and position updates.

//    tpktHeader (4 bytes): A TPKT Header, as specified in [T123] section 8.

//    x224Data (3 bytes): An X.224 Class 0 Data TPDU, as specified in [X224]
//      section 13.7.

//    mcsSDin (variable): Variable-length PER-encoded MCS Domain PDU which
//      encapsulates an MCS Send Data Indication structure, as specified in
//      [T125] (the ASN.1 structure definitions are given in [T125] section 7,
//      parts 7 and 10). The userData field of the MCS Send Data Indication
//      contains a Security Header and the Pointer Update PDU data.

//    securityHeader (variable): Optional security header. If the Encryption
//      Level (sections 5.3.2 and 2.2.1.4.3) selected by the server is greater
//      than ENCRYPTION_LEVEL_NONE (0) and the Encryption Method
//      (sections 5.3.2 and 2.2.1.4.3) selected by the server is greater than
//      ENCRYPTION_METHOD_NONE (0) then this field will contain one of the
//      following headers:

//      - Basic Security Header (section 2.2.8.1.1.2.1) if the Encryption Level
//        selected by the server (see sections 5.3.2 and 2.2.1.4.3) is
//        ENCRYPTION_LEVEL_LOW (1).

//      - Non-FIPS Security Header (section 2.2.8.1.1.2.2) if the Encryption
//        Level selected by the server (see sections 5.3.2 and 2.2.1.4.3) is
//        ENCRYPTION_LEVEL_CLIENT_COMPATIBLE (2) or ENCRYPTION_LEVEL_HIGH (3).

//      - FIPS Security Header (section 2.2.8.1.1.2.3) if the Encryption Level
//        selected by the server (see sections 5.3.2 and 2.2.1.4.3) is
//        ENCRYPTION_LEVEL_FIPS (4).

//      If the Encryption Level (sections 5.3.2 and 2.2.1.4.3) selected by the
//      server is ENCRYPTION_LEVEL_NONE (0) and the Encryption Method (sections
//      5.3.2 and 2.2.1.4.3) selected by the server is ENCRYPTION_METHOD_NONE
//      (0), then this header is not include " in the PDU.

//    shareDataHeader (18 bytes): Share Data Header (section 2.2.8.1.1.1.2)
//      containing information about the packet. The type subfield of the
//      pduType field of the Share Control Header (section 2.2.8.1.1.1.1) MUST
//      be set to PDUTYPE_DATAPDU (7). The pduType2 field of the Share Data
//      Header MUST be set to PDUTYPE2_POINTER (27).

//    messageType (2 bytes): A 16-bit, unsigned integer. Type of pointer update.

//    +--------------------------------+---------------------------------------+
//    | 0x0001 TS_PTRMSGTYPE_SYSTEM    | Indicates a System Pointer Update     |
//    |                                | (section 2.2.9.1.1.4.3).              |
//    +--------------------------------+---------------------------------------+
//    | 0x0003 TS_PTRMSGTYPE_POSITION  | Indicates a Pointer Position Update   |
//    |                                | (section 2.2.9.1.1.4.2).              |
//    +--------------------------------+---------------------------------------+
//    | 0x0006 TS_PTRMSGTYPE_COLOR     | Indicates a Color Pointer Update      |
//    |                                | (section 2.2.9.1.1.4.4).              |
//    +--------------------------------+---------------------------------------+
//    | 0x0007 TS_PTRMSGTYPE_CACHED    | Indicates a Cached Pointer Update     |
//    |                                | (section 2.2.9.1.1.4.6).              |
//    +--------------------------------+---------------------------------------+
//    | 0x0008 TS_PTRMSGTYPE_POINTER   | Indicates a New Pointer Update        |
//    |                                | (section 2.2.9.1.1.4.5).              |
//    +--------------------------------+---------------------------------------+


//    2.2.9.1.1.4.2     Pointer Position Update (TS_POINTERPOSATTRIBUTE)
//    -------------------------------------------------------------------
//    The TS_POINTERPOSATTRIBUTE structure is used to indicate that the client
//    pointer should be moved to the specified position relative to the top-left
//    corner of the server's desktop (see [T128] section 8.14.4).

//    position (4 bytes): Point (section 2.2.9.1.1.4.1) structure containing
//     the new x-coordinates and y-coordinates of the pointer.
//            2.2.9.1.1.4.1  Point (TS_POINT16)
//            ---------------------------------
//            The TS_POINT16 structure specifies a point relative to the
//            top-left corner of the server's desktop.

//            xPos (2 bytes): A 16-bit, unsigned integer. The x-coordinate
//              relative to the top-left corner of the server's desktop.

//            yPos (2 bytes): A 16-bit, unsigned integer. The y-coordinate
//              relative to the top-left corner of the server's desktop.



//    2.2.9.1.1.4.3     System Pointer Update (TS_SYSTEMPOINTERATTRIBUTE)
//    -------------------------------------------------------------------
//    The TS_SYSTEMPOINTERATTRIBUTE structure is used to hide the pointer or to
//    set its shape to that of the operating system default (see [T128] section
//    8.14.1).

//    systemPointerType (4 bytes): A 32-bit, unsigned integer.
//    The type of system pointer.
//    +---------------------------|------------------------------+
//    | 0x00000000 SYSPTR_NULL    | The hidden pointer.          |
//    +---------------------------|------------------------------+
//    | 0x00007F00 SYSPTR_DEFAULT | The default system pointer.  |
//    +---------------------------|------------------------------+

    void send_pointer(uint16_t cache_idx, RdpPointerView const& cursor)
    {
        LOG_IF(bool(this->verbose & RDPSerializerVerbose::pointer), LOG_INFO,
            "GraphicsUpdatePDU::send_pointer(cache_idx=%u)", cache_idx);

        assert(cursor.xor_bits_per_pixel() != BitsPerPixel{0});

        StaticOutReservedStreamHelper<1024, 65536-1024> stream;
        bool new_pointer_update_used = emit_native_pointer(stream.get_data_stream(), cache_idx, cursor);

        this->send_pointer_impl(
            new_pointer_update_used
                ? FastPath::UpdateType::POINTER
                : FastPath::UpdateType::COLOR,
            new_pointer_update_used
                ? RDP_POINTER_NEW
                : RDP_POINTER_COLOR,
            stream);

        LOG_IF(bool(this->verbose & RDPSerializerVerbose::pointer), LOG_INFO,
            "GraphicsUpdatePDU::send_pointer done");
    }   // void send_pointer(int cache_idx, const Pointer & cursor)


//    2.2.9.1.1.4.6    Cached Pointer Update (TS_CACHEDPOINTERATTRIBUTE)
//    ------------------------------------------------------------------
//    The TS_CACHEDPOINTERATTRIBUTE structure is used to instruct the client to
//    change the current pointer shape to one already present in the pointer
//    cache.

//    cacheIndex (2 bytes): A 16-bit, unsigned integer. A zero-based cache entry
//      containing the cache index of the cached pointer to which the client's
//      pointer should be changed. The pointer data should have already been
//      cached using either the Color Pointer Update (section 2.2.9.1.1.4.4) or
//      New Pointer Update (section 2.2.9.1.1.4.5).

    void cached_pointer_update(uint16_t cache_idx)
    {
        LOG_IF(bool(this->verbose & RDPSerializerVerbose::pointer), LOG_INFO,
            "GraphicsUpdatePDU::cached_pointer_update(cache_idx=%d)", cache_idx);

        StaticOutReservedStreamHelper<1024, 65536-1024> stream;
        stream.get_data_stream().out_uint16_le(cache_idx);

        this->send_pointer_impl(
            FastPath::UpdateType::CACHED,
            RDP_POINTER_CACHED,
            stream);

        LOG_IF(bool(this->verbose & RDPSerializerVerbose::pointer), LOG_INFO,
            "GraphicsUpdatePDU::cached_pointer_update done");
    }   // void cached_pointer_update(int cache_idx)

    void send_native_pointer(PredefinedPointer predefined_pointer)
    {
        LOG_IF(bool(this->verbose & RDPSerializerVerbose::pointer), LOG_INFO,
            "GraphicsUpdatePDU::send_native_pointer(cache_idx=%u)",
            unsigned(predefined_pointer));

        StaticOutReservedStreamHelper<1024, 126> stream;
        bool is_null_pointer = (predefined_pointer == PredefinedPointer::Null);

        if (!this->fastpath_support) {
            stream.get_data_stream().out_uint32_le(
                is_null_pointer
                    ? RDP_NULL_POINTER
                    : RDP_DEFAULT_POINTER
            );
        }

        this->send_pointer_impl(
            is_null_pointer
                ? FastPath::UpdateType::PTR_NULL
                : FastPath::UpdateType::PTR_DEFAULT,
            RDP_POINTER_SYSTEM,
            stream);

        LOG_IF(bool(this->verbose & RDPSerializerVerbose::pointer), LOG_INFO,
            "GraphicsUpdatePDU::send_native_pointer done");
    }

    inline void send_pointer_impl(
        FastPath::UpdateType updateCode,
        uint16_t updateType,
        OutReservedStreamHelper& stream)
    {
        if (fastpath_support) {
            send_fast_path_data(
                this->trans, this->compression, this->mppc_enc,
                this->encryptionLevel, this->encrypt,
                updateCode, stream);
        }
        else {
            StaticOutStream<64> data;

            data.out_uint16_le(updateType);
            data.out_clear_bytes(2);

            stream.copy_to_head(data.get_produced_bytes());

            send_share_data_ex(
                this->trans, PDUTYPE2_POINTER, this->compression,
                this->mppc_enc, this->shareid, this->encryptionLevel,
                this->encrypt, this->userid, stream,
                bool(this->verbose & (RDPSerializerVerbose::pointer
                                    | RDPSerializerVerbose::sec_decrypted)));
        }
    }

public:
    void cached_pointer(gdi::CachePointerIndex cache_idx) override
    {
        if (cache_idx.is_predefined_pointer()
         && (cache_idx.as_predefined_pointer() == PredefinedPointer::SystemNormal
          || cache_idx.as_predefined_pointer() == PredefinedPointer::Null
        )) {
            this->send_native_pointer(cache_idx.as_predefined_pointer());
        }
        else {
            auto result_cache = this->pointer_cache.use(cache_idx);
            auto idx = result_cache.destination_idx;

            if (result_cache.is_cached) {
                this->cached_pointer_update(idx);
            }
            else {
                this->send_pointer(idx, this->pointer_cache.pointer(cache_idx));
            }
        }
    }

    void new_pointer(gdi::CachePointerIndex cache_idx, RdpPointerView const& cursor) override
    {
        auto idx = this->pointer_cache.insert_and_use(cache_idx, cursor);
        this->send_pointer(idx, cursor);
    }

    void send_set_surface_command(RDPSetSurfaceCommand const & cmd) {
        LOG_IF(bool(this->verbose & RDPSerializerVerbose::surface_commands), LOG_INFO,
            "GraphicsUpdatePDU::send_surface_command");

        BufMaker<65536> buf_maker;
        std::size_t headen_len = 1024;
        auto buffer = buf_maker.dyn_array(headen_len + 1024 + cmd.bitmapDataLength);

        OutReservedStreamHelper stream(buffer, headen_len);
        stream.get_data_stream().out_uint16_le(0x0001); // CMDTYPE_SET_SURFACE_BITS
        cmd.emit(stream.get_data_stream());

        send_fast_path_data(
            this->trans, this->compression, this->mppc_enc,
            this->encryptionLevel, this->encrypt,
            FastPath::UpdateType::SURFCMDS, stream);
    }

    void update_pointer_position(uint16_t xPos, uint16_t yPos)
    {
        LOG_IF(bool(this->verbose & RDPSerializerVerbose::pointer), LOG_INFO,
            "GraphicsUpdatePDU::update_pointer_position(xPos=%u, yPos=%u)", xPos, yPos);

        StaticOutReservedStreamHelper<1024, 64> stream;
        stream.get_data_stream().out_uint16_le(xPos);
        stream.get_data_stream().out_uint16_le(yPos);

        this->send_pointer_impl(
            FastPath::UpdateType::PTR_POSITION,
            RDP_POINTER_MOVE,
            stream);

        LOG_IF(bool(this->verbose & RDPSerializerVerbose::pointer), LOG_INFO,
            "GraphicsUpdatePDU::update_pointer_position done");
    }
};
