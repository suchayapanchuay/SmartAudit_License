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

#include "core/RDP/orders/RDPOrdersCommon.hpp"

// 2.2.2.2.1.1.1.1 Coord Field (COORD_FIELD)
// =========================================
// The COORD_FIELD structure is used to describe a value in the range -32768
//  to 32767.

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |     signedValue (variable)    |
// +-------------------------------+

// signedValue (variable): A signed, 1-byte or 2-byte value that describes a
//  coordinate in the range -32768 to 32767.

//  When the controlFlags field (see section 2.2.2.2.1.1.2) of the primary
//   drawing order that contains the COORD_FIELD structure has the
//   TS_DELTA_COORDINATES flag (0x10) set, the signedValue field MUST contain
//   a signed 1-byte value. If the TS_DELTA_COORDINATES flag is not set, the
//   signedValue field MUST contain a 2-byte signed value.

//  The 1-byte format contains a signed delta from the previous value of the
//   Coord field. To obtain the new value of the field, the decoder MUST
//   increment the previous value of the field by the signed delta to produce
//   the current value. The 2-byte format is simply the full value of the
//   field that MUST replace the previous value.

// 2.2.2.2.1.1.1.2 One-Byte Header Variable Field (VARIABLE1_FIELD)
// ================================================================
// TheVARIABLE1_FIELD structure is used to encode a variable-length byte-
//  stream that will hold a maximum of 255 bytes. This structure is always
//  situated at the end of an order.

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |     cbData    |               rgbData (variable)              |
// +---------------+-----------------------------------------------+
// |                              ...                              |
// +---------------------------------------------------------------+

// cbData (1 byte): An 8-bit, unsigned integer. The number of bytes present
//  in the rgbData field.

// rgbData (variable): Variable-length, binary data. The size of this data,
//  in bytes, is given by the cbData field.

// 2.2.2.2.1.1.1.4 Delta-Encoded Points (DELTA_PTS_FIELD)
// ======================================================
// The DELTA_PTS_FIELD structure is used to encode a series of points. Each
//  point is expressed as an X and Y delta from the previous point in the
//  series (the first X and Y deltas are relative to a base point that MUST
//  be included in the order that contains the DELTA_PTS_FIELD structure).
//  The number of points is order-dependent and is not specified by any field
//  within the DELTA_PTS_FIELD structure. Instead, a separate field within
//  the order that contains the DELTA_PTS_FIELD structure MUST be used to
//  specify the number of points (this field SHOULD<1> be placed immediately
//  before the DELTA_PTS_FIELD structure in the order encoding).

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      zeroBits (variable)                      |
// +---------------------------------------------------------------+
// |                              ...                              |
// +---------------------------------------------------------------+
// |                 deltaEncodedPoints (variable)                 |
// +---------------------------------------------------------------+
// |                              ...                              |
// +---------------------------------------------------------------+

// zeroBits (variable): A variable-length byte field. The zeroBits field is
//  used to indicate the absence of an X or Y delta value for a specific
//  point in the series. The size in bytes of the zeroBits field is given by
//  ceil(NumPoints / 4) where NumPoints is the number of points being
//  encoded. Each point in the series requires two zero-bits (four points per
//  byte) to indicate whether an X or Y delta value is zero (and not
//  present), starting with the most significant bits, so that for the first
//  point the X-zero flag is set at (zeroBits[0] & 0x80), and the Y-zero flag
//  is set at (zeroBits[0] & 0x40).

// deltaEncodedPoints (variable): A variable-length byte field. The
//  deltaEncodedPoints field contains a series of (X, Y) pairs, each pair
//  specifying the delta from the previous pair in the series (the first pair
//  in the series contains a delta from a pre-established coordinate).

//  The presence of the X and Y delta values for a given pair in the series
//   is dictated by the individual bits of the zeroBits field. If the zero
//   bit is set for a given X or Y component, its value is unchanged from the
//   previous X or Y component in the series (a delta of zero), and no data
//   is provided. If the zero bit is not set for a given X or Y component,
//   the delta value it represents is encoded in a packed signed format:

//   * If the high bit (0x80) is not set in the first encoding byte, the
//     field is 1 byte long and is encoded as a signed delta in the lower 7
//     bits of the byte.

//   * If the high bit of the first encoding byte is set, the lower 7 bits of
//     the first byte and the 8 bits of the next byte are concatenated (the
//     first byte containing the high-order bits) to create a 15-bit signed
//     delta value.

// 2.2.2.2.1.1.2.16 PolygonSC (POLYGON_SC_ORDER)
// =============================================
// The PolygonSC Primary Drawing Order encodes a solid-color polygon
// consisting of two or more vertices connected by straight lines.

// Encoding order number: 20 (0x14)
// Negotiation order number: 20 (0x14)
// Number of fields: 7
// Number of field encoding bytes: 1
// Maximum encoded field length: 249 bytes

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |       xStart (variable)       |       yStart (variable)       |
// +---------------+---------------+---------------+---------------+
// |     bRop2     |    FillMode   |         Brush Color           |
// |   (optional)  |   (optional)  |          (optional)           |
// +---------------+---------------+---------------+---------------+
// |      ...      |NumDeltaEntries|        CodedDeltaList         |
// |               |   (optional)  |          (variable)           |
// +-------------------------------+---------------+---------------+

// xStart (variable): The x-coordinate of the starting point of the polygon
//  path specified by using a Coord Field (section 2.2.2.2.1.1.1.1).
// yStart (variable): The y-coordinate of the starting point of the polygon
//  path specified by using a Coord Field (section 2.2.2.2.1.1.1.1).
// bRop2 (1 byte): The binary raster operation to perform (see section 2.2.2.2.1.1.1.6).

// FillMode (1 byte): The polygon filling algorithm described using a
//  Fill Mode (section 2.2.2.2.1.1.1.9) structure.
// BrushColor (3 bytes): Foreground color described by using a
//  Generic Color (section 2.2.2.2.1.1.1.8) structure.
// NumDeltaEntries (1 byte): An 8-bit, unsigned integer. The number of points
//  along the polygon path described by the CodedDeltaList field.
// CodedDeltaList (variable): A One-Byte Header Variable Field (section 2.2.2.2.1.1.1.2)
//  structure that encapsulates a Delta-Encoded Points (section 2.2.2.2.1.1.1.4)
//  structure that contains the points along the polygon path.
//  The number of points described by the Delta-Encoded Points structure
//  is specified by the NumDeltaEntries field.

class RDPPolygonSC {
public:
    int16_t  xStart{0};
    int16_t  yStart{0};
    uint8_t  bRop2{0x0};
    uint8_t  fillMode{0x00};
    RDPColor BrushColor;
    uint8_t  NumDeltaEntries{0};

    struct DeltaPoint {
        int16_t xDelta;
        int16_t yDelta;
    } deltaPoints [128] = {};

    static uint8_t id() {
        return RDP::POLYGONSC;
    }

    RDPPolygonSC() = default;

    RDPPolygonSC(int16_t xStart, int16_t yStart, uint8_t bRop2, uint8_t fillMode,
                 RDPColor BrushColor, uint8_t NumDeltaEntries, InStream & deltaPoints)
    : xStart         (xStart)
    , yStart         (yStart)
    , bRop2          (bRop2)
    , fillMode       (fillMode)
    , BrushColor     (BrushColor)
    , NumDeltaEntries(std::min<uint8_t>(NumDeltaEntries, std::size(this->deltaPoints)))
    {
        for (int i = 0; i < this->NumDeltaEntries; i++) {
            this->deltaPoints[i].xDelta = deltaPoints.in_sint16_le();
            this->deltaPoints[i].yDelta = deltaPoints.in_sint16_le();
        }
    }

    bool operator==(const RDPPolygonSC & other) const {
        return  (this->xStart          == other.xStart)
             && (this->yStart          == other.yStart)
             && (this->bRop2           == other.bRop2)
             && (this->fillMode        == other.fillMode)
             && (this->BrushColor      == other.BrushColor)
             && (this->NumDeltaEntries == other.NumDeltaEntries)
             && !memcmp(this->deltaPoints, other.deltaPoints, sizeof(DeltaPoint) * this->NumDeltaEntries)
             ;
    }

    void emit(OutStream & stream, RDPOrderCommon & common, const RDPOrderCommon & oldcommon,
              const RDPPolygonSC & oldcmd) const {
        RDPPrimaryOrderHeader header(RDP::STANDARD, 0);

        // TODO check that
        int16_t pointx = this->xStart;
        int16_t pointy = this->yStart;
        if (!common.clip.contains_pt(pointx, pointy)) {
            header.control |= RDP::BOUNDS;
        }
        else {
            for (uint8_t i = 0; i < this->NumDeltaEntries; i++) {
                pointx += this->deltaPoints[i].xDelta;
                pointy += this->deltaPoints[i].yDelta;

                if (!common.clip.contains_pt(pointx, pointy)) {
                    header.control |= RDP::BOUNDS;
                    break;
                }
            }
        }

        header.control |= (is_1_byte(this->xStart - oldcmd.xStart) && is_1_byte(this->yStart - oldcmd.yStart)) * RDP::DELTA;

        header.fields =
                (this->xStart          != oldcmd.xStart         ) * 0x0001
              | (this->yStart          != oldcmd.yStart         ) * 0x0002
              | (this->bRop2           != oldcmd.bRop2          ) * 0x0004
              | (this->fillMode        != oldcmd.fillMode       ) * 0x0008
              | (this->BrushColor      != oldcmd.BrushColor     ) * 0x0010
              | (this->NumDeltaEntries != oldcmd.NumDeltaEntries) * 0x0020
              | (
                 (this->NumDeltaEntries != oldcmd.NumDeltaEntries) ||
                 0 != memcmp(this->deltaPoints, oldcmd.deltaPoints,
                             this->NumDeltaEntries * sizeof(DeltaPoint))
                ) * 0x0040
              ;

        common.emit(stream, header, oldcommon);

        header.emit_coord(stream, 0x0001, this->xStart, oldcmd.xStart);
        header.emit_coord(stream, 0x0002, this->yStart, oldcmd.yStart);

        if (header.fields & 0x0004) { stream.out_uint8(this->bRop2); }

        if (header.fields & 0x0008) { stream.out_uint8(this->fillMode); }

        if (header.fields & 0x0010) {
            emit_rdp_color(stream, this->BrushColor);
        }

        if (header.fields & 0x0020) { stream.out_uint8(this->NumDeltaEntries); }

        if (header.fields & 0x0040) {
            uint32_t offset_cbData = stream.get_offset();
            stream.out_clear_bytes(1);

            uint8_t * zeroBit = stream.get_current();
            stream.out_clear_bytes((this->NumDeltaEntries + 3) / 4);

            *zeroBit = 0;
            for (uint8_t i = 0, m4 = 0; i < this->NumDeltaEntries; i++, m4++) {
                if (m4 == 4) {
                    m4 = 0;
                }

                if (i && !m4) {
                    *(++zeroBit) = 0;
                }

                if (!this->deltaPoints[i].xDelta) {
                    *zeroBit |= (1 << (7 - m4 * 2));
                }
                else {
                    stream.out_DEP(this->deltaPoints[i].xDelta);
                }

                if (!this->deltaPoints[i].yDelta) {
                    *zeroBit |= (1 << (6 - m4 * 2));
                }
                else {
                    stream.out_DEP(this->deltaPoints[i].yDelta);
                }
            }

            stream.stream_at(offset_cbData).out_uint8(stream.get_offset() - offset_cbData - 1);
        }
    }

    void receive(InStream & stream, const RDPPrimaryOrderHeader & header) {
        // LOG(LOG_INFO, "RDPPolygonSC::receive: header fields=0x%02X", header.fields);

        header.receive_coord(stream, 0x0001, this->xStart);
        header.receive_coord(stream, 0x0002, this->yStart);
        if (header.fields & 0x0004) {
            this->bRop2  = stream.in_uint8();
        }
        if (header.fields & 0x0008) {
            this->fillMode = stream.in_uint8();
        }

        if (header.fields & 0x0010) {
            receive_rdp_color(stream, this->BrushColor);
        }

        if (header.fields & 0x0020) {
            this->NumDeltaEntries = stream.in_uint8();
        }

        if (header.fields & 0x0040) {
            uint8_t cbData = stream.in_uint8();
            // LOG(LOG_INFO, "cbData=%d", cbData);

            InStream rgbData(stream.in_skip_bytes(cbData));
            //hexdump_d(rgbData.get_current(), rgbData.get_capacity());

            uint8_t zeroBitsSize = ((this->NumDeltaEntries + 3) / 4);
            // LOG(LOG_INFO, "zeroBitsSize=%d", zeroBitsSize);

            InStream zeroBits(rgbData.in_skip_bytes(zeroBitsSize));

            uint8_t zeroBit = 0;

            for (uint8_t i = 0, m4 = 0; i < this->NumDeltaEntries; i++, m4++) {
                if (m4 == 4) {
                    m4 = 0;
                }

                if (!m4) {
                    zeroBit = zeroBits.in_uint8();
                    // LOG(LOG_INFO, "0x%02X", zeroBit);
                }

                this->deltaPoints[i].xDelta = (!(zeroBit & 0x80) ? rgbData.in_DEP() : 0);
                this->deltaPoints[i].yDelta = (!(zeroBit & 0x40) ? rgbData.in_DEP() : 0);

/*
                LOG(LOG_INFO, "RDPPolygonSC::receive: delta point=%d, %d",
                    this->deltaPoints[i].xDelta, this->deltaPoints[i].yDelta);
*/

                zeroBit <<= 2;
            }
        }
    }   // receive

    size_t str(char * buffer, size_t sz, const RDPOrderCommon & common) const {
        size_t lg = 0;
        lg += common.str(buffer + lg, sz - lg, true);
        lg += snprintf(buffer + lg, sz - lg,
            "polygonsc(xStart=%d yStart=%d bRop2=0x%02X fillMode=%d BrushColor=%.6x "
                "NumDeltaEntries=%d DeltaEntries=(",
            this->xStart, this->yStart, unsigned(this->bRop2),
            this->fillMode, this->BrushColor.as_bgr().as_u32(), this->NumDeltaEntries);
        for (uint8_t i = 0; i < this->NumDeltaEntries; i++) {
            if (i) {
                lg += snprintf(buffer + lg, sz - lg, " ");
            }
            lg += snprintf(buffer + lg, sz - lg, "(%d, %d)", this->deltaPoints[i].xDelta, this->deltaPoints[i].yDelta);
        }
        lg += snprintf(buffer + lg, sz - lg, "))");
        if (lg >= sz) {
            return sz;
        }
        return lg;
    }

    void log(int level, const Rect clip) const {
        char buffer[2048];
        this->str(buffer, sizeof(buffer), RDPOrderCommon(this->id(), clip));
        buffer[sizeof(buffer) - 1] = 0;
        LOG(level, "%s", buffer);
    }
};  // class RDPPolygonSC

