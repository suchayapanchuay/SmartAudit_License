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

#include "utils/ellipse.hpp"
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

// 2.2.2.2.1.1.2.20 EllipseCB (ELLIPSE_CB_ORDER)
// =============================================
// The EllipseCB Primary Drawing Order encodes a color brush ellipse.

// Encoding order number: 26 (0x1A)
// Negotiation order number: 26 (0x1A)
// Number of fields: 13
// Number of field encoding bytes: 2
// Maximum encoded field length: 27 bytes

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |      LeftRect (variable)      |      TopRect (variable)       |
// +---------------+---------------+---------------+---------------+
// |      RightRect (variable)     |     BottomRect (variable)     |
// +---------------+---------------+---------------+---------------+
// |     bRop2     |    FillMode   |         BackColor             |
// |   (optional)  |   (optional)  |         (optional)            |
// +---------------+---------------+---------------+---------------+
// |      ...      |             ForeColor (optional)              |
// +---------------+---------------+---------------+---------------+
// |  BrushOrgX    |  BrushOrgY    |  BrushStyle   |  BrushHatch   |
// |  (optional)   |  (optional)   |  (optional)   |  (optional)   |
// +---------------+---------------+---------------+---------------+
// |                     BrushExtra (optional)                     |
// +-----------------------------------------------+---------------+
// |                      ...                      |
// +-----------------------------------------------+

// LeftRect (variable): The left coordinate of the inclusive rectangle for
//  the ellipse specified by using a Coord Field (section 2.2.2.2.1.1.1.1).

// TopRect (variable): The top coordinate of the inclusive rectangle for
//  the ellipse specified by using a Coord Field (section 2.2.2.2.1.1.1.1).

// RightRect (variable): The right coordinate of the inclusive rectangle for
//  the ellipse specified by using a Coord Field (section 2.2.2.2.1.1.1.1).

// BottomRect (variable): The bottom coordinate of the inclusive rectangle for
//  the ellipse specified by using a Coord Field (section 2.2.2.2.1.1.1.1).

// bRop2 (1 byte): The binary raster operation to perform
//  (see section 2.2.2.2.1.1.1.6).

// FillMode (1 byte): An 8-bit, unsigned integer that specifies the fill mode.
//  FillMode MUST be one of the following values.

// +-----------+----------------------------------------------------+
// | Value     | Meaning                                            |
// +-----------+----------------------------------------------------+
// | ALTERNATE | See section 2.2.2.2.1.1.1.9 for an explanation of  |
// | 0x01      | this value.                                        |
// +-----------+----------------------------------------------------+
// | WINDING   | See section 2.2.2.2.1.1.1.9 for an explanation of  |
// | 0x02      | this value.                                        |
// +-----------+----------------------------------------------------+

// BackColor (3 bytes): The background color described by using a Generic Color
//  (section 2.2.2.2.1.1.1.8) structure.

// ForeColor (3 bytes): The foreground color described by using a Generic Color
//  (section 2.2.2.2.1.1.1.8) structure.

// BrushOrgX (1 byte): An 8-bit, signed integer. The x-coordinate of the point
//  where the top leftmost pixel of a brush pattern MUST be anchored.

// BrushOrgY (1 byte): An 8-bit, signed integer. The y-coordinate of the point
//  where the top leftmost pixel of a brush pattern MUST be anchored.

// BrushStyle (1 byte): An 8-bit, unsigned integer. The contents and format of
//  this field are the same as the BrushStyle field of the PatBlt
//  (section 2.2.2.2.1.1.2.3) Primary Drawing Order.

// BrushHatch (1 byte): An 8-bit, unsigned integer. The contents and format of
//  this field are the same as the BrushHatch field of the PatBlt
//  (section 2.2.2.2.1.1.2.3) Primary Drawing Order.

// BrushExtra (7 bytes): A byte array of length 7. The contents and format of
//  this field are the same as the BrushExtra field of the PatBlt
//  (section 2.2.2.2.1.1.2.3) Primary Drawing Order.


class RDPEllipseCB {
public:
    Ellipse el;
    uint8_t  brop2;
    uint8_t  fill_mode;
    RDPColor back_color;
    RDPColor fore_color;
    RDPBrush brush;

    static uint8_t id()
    {
        return RDP::ELLIPSECB;
    }

    RDPEllipseCB(const Rect rect, uint8_t rop, uint8_t fill,
                 RDPColor back_color, RDPColor fore_color,
                 const RDPBrush & brush) :
        el(rect),
        brop2(rop),
        fill_mode(fill),
        back_color(back_color),
        fore_color(fore_color),
        brush(brush)
    {
    }

    bool operator==(const RDPEllipseCB & other) const {
        return (this->el == other.el
            && (this->brop2 == other.brop2)
            && (this->fill_mode == other.fill_mode)
            && (this->back_color == other.back_color)
            && (this->fore_color == other.fore_color)
            && (this->brush == other.brush));
    }

    void emit(OutStream & stream,
              RDPOrderCommon & common,
              const RDPOrderCommon & oldcommon,
              const RDPEllipseCB & oldcmd) const
    {
        using namespace RDP;
        RDPPrimaryOrderHeader header(STANDARD, 0);

        if (!common.clip.contains(this->el.get_rect())){
            header.control |= BOUNDS;
        }

        int16_t left      = this->el.ileft();
        int16_t top       = this->el.itop();
        int16_t right     = this->el.eright();
        int16_t bottom    = this->el.ebottom();
        int16_t oldleft   = oldcmd.el.ileft();
        int16_t oldtop    = oldcmd.el.itop();
        int16_t oldright  = oldcmd.el.eright();
        int16_t oldbottom = oldcmd.el.ebottom();

        header.control |= (is_1_byte(left - oldleft) &&
                           is_1_byte(top - oldtop) &&
                           is_1_byte(right - oldright) &&
                           is_1_byte(bottom - oldbottom)) * RDP::DELTA;
        header.fields =
            ( left              != oldleft           ) * 0x0001
            |(top               != oldtop            ) * 0x0002
            |(right             != oldright          ) * 0x0004
            |(bottom            != oldbottom         ) * 0x0008
            |(this->brop2       != oldcmd.brop2      ) * 0x0010
            |(this->fill_mode   != oldcmd.fill_mode  ) * 0x0020
            |(this->back_color  != oldcmd.back_color ) * 0x0040
            |(this->fore_color  != oldcmd.fore_color ) * 0x0080
            |(this->brush.org_x != oldcmd.brush.org_x) * 0x0100
            |(this->brush.org_y != oldcmd.brush.org_y) * 0x0200
            |(this->brush.style != oldcmd.brush.style) * 0x0400
            |(this->brush.hatch != oldcmd.brush.hatch) * 0x0800
            |(memcmp(this->brush.extra, oldcmd.brush.extra, 7) != 0) * 0x1000;

        common.emit(stream, header, oldcommon);
        header.emit_coord(stream, 0x0001, left,   oldleft);
        header.emit_coord(stream, 0x0002, top,    oldtop);
        header.emit_coord(stream, 0x0004, right,  oldright);
        header.emit_coord(stream, 0x0008, bottom, oldbottom);

        if (header.fields & 0x0010) { stream.out_uint8(this->brop2); }

        if (header.fields & 0x0020) { stream.out_uint8(this->fill_mode); }

        if (header.fields & 0x0040) {
            emit_rdp_color(stream, this->back_color);
        }
        if (header.fields & 0x0080) {
            emit_rdp_color(stream, this->fore_color);
        }
        header.emit_brush(stream, 0x0100, this->brush, oldcmd.brush);
    }

    void receive(InStream & stream, const RDPPrimaryOrderHeader & header)
    {
        // using namespace RDP;
        int16_t  leftRect   = this->el.ileft();
        int16_t  topRect    = this->el.itop();
        int16_t  rightRect  = this->el.eright();
        int16_t  bottomRect = this->el.ebottom();
        header.receive_coord(stream, 0x0001, leftRect);
        header.receive_coord(stream, 0x0002, topRect);
        header.receive_coord(stream, 0x0004, rightRect);
        header.receive_coord(stream, 0x0008, bottomRect);

        this->el = Ellipse(leftRect, topRect, rightRect, bottomRect);

        if (header.fields & 0x0010) {
            this->brop2  = stream.in_uint8();
        }
        if (header.fields & 0x0020) {
            this->fill_mode  = stream.in_uint8();
        }
        if (header.fields & 0x0040) {
            receive_rdp_color(stream, this->back_color);
        }
        if (header.fields & 0x0080) {
            receive_rdp_color(stream, this->fore_color);
        }

        header.receive_brush(stream, 0x0100, this->brush);
    }

    size_t str(char * buffer, size_t sz, const RDPOrderCommon & common) const {
        size_t lg = 0;
        lg += common.str(buffer + lg, sz - lg, true);
        lg += snprintf(buffer + lg, sz - lg,
                       "ellipseCB(leftRect=%d topRect=%d rightRect=%d bottomRect=%d bRop2=0x%02X "
                       "fillMode=%d backColor=%.6x foreColor=%.6x"
                       "brush.org_x=%d brush.org_y=%d "
                       "brush.style=%d brush.hatch=%d)",
                       this->el.ileft(), this->el.itop(), this->el.eright(), this->el.ebottom(),
                       unsigned(this->brop2), this->fill_mode, this->back_color.as_bgr().as_u32(), this->fore_color.as_bgr().as_u32(),
                       this->brush.org_x, this->brush.org_y,
                       this->brush.style, this->brush.hatch);
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
};
