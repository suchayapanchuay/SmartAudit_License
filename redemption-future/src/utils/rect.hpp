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
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni, Meng Tan
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Rect class : Copyright (C) Christophe Grosjean 2009

*/


#pragma once

#include "utils/sugar/std_stream_proto.hpp"

#include <iosfwd>
#include <algorithm> // min max
#include <cstdlib> // abs
#include <cstdint>
#include <cassert>
#include <cstdio> // sprintf


struct Point
{
    int16_t x;
    int16_t y;

    REDEMPTION_FRIEND_OSTREAM(out, Point const & p)
    {
        return out << "(" << p.x << ", " << p.y << ")";
    }

    constexpr bool operator==(Point const & other) const noexcept
    {
        return (other.x == this->x
             && other.y == this->y);
    }
};


struct Dimension
{
    uint16_t w = 0;
    uint16_t h = 0;

    constexpr Dimension() = default;

    constexpr Dimension(uint16_t w, uint16_t h) noexcept
        : w(w)
        , h(h)
    {}

    constexpr uint16_t width() const noexcept
    {
        return this->w;
    }

    constexpr uint16_t height() const noexcept
    {
        return this->h;
    }

    constexpr bool operator==(Dimension const & other) const noexcept
    {
        return (other.w == this->w
             && other.h == this->h);
    }

    constexpr bool isempty() const noexcept
    {
        return (!this->w || !this->h);
    }

    REDEMPTION_FRIEND_OSTREAM(out, Dimension const & d)
    {
        return out << "(" << d.w << ", " << d.h << ")";
    }
};

inline auto log_value(Dimension const & dim) noexcept
{
    struct {
        char value[64];
    } d;
    ::sprintf(d.value, "Dimension(%u %u)", dim.w, dim.h);
    return d;
}


struct Rect
{
    int16_t x = 0;
    int16_t y = 0;
    uint16_t cx = 0;
    uint16_t cy = 0;

    // Inclusive left of rectangle
    constexpr int16_t ileft() const noexcept
    {
        return this->x;
    }

    // Exclusive right of rectangle
    constexpr int16_t eright() const noexcept
    {
        return static_cast<int16_t>(this->x + this->cx);
    }

    constexpr int16_t itop() const noexcept
    {
        return this->y;
    }

    constexpr int16_t ebottom() const noexcept
    {
        return static_cast<int16_t>(this->y + this->cy);
    }

    constexpr uint16_t width() const noexcept
    {
        return this->cx;
    }

    constexpr uint16_t height() const noexcept
    {
        return this->cy;
    }

    constexpr Rect() = default;

    constexpr Rect(int16_t left, int16_t top, uint16_t width, uint16_t height) noexcept
        : x(left), y(top), cx(width), cy(height)
    {
        // fast detection of overflow, works for valid width/height range 0..32768
        if (((width-1)|(height-1)) & 0x8000){
            this->cx = 0;
            this->cy = 0;
        }
    }

    constexpr Rect(Point point, Dimension dimension) noexcept
        : Rect(point.x, point.y, dimension.w, dimension.h)
    {}

    constexpr bool contains_pt(int16_t x, int16_t y) const noexcept
    {
        // return this->contains(Rect(x,y,1,1));
        return x >= this->x
            && y >= this->y
            && x  < this->x + this->cx
            && y  < this->y + this->cy;
    }

    constexpr bool has_intersection(int16_t x, int16_t y) const noexcept
    {
        return this->cx && this->cy
            && (x >= this->x && x < this->eright())
            && (y >= this->y && y < this->ebottom());
    }

    // special cases: contains returns true
    // - if both rects are empty
    // - if inner rect is empty
    constexpr bool contains(Rect inner) const noexcept
    {
        return (inner.x >= this->x
              && inner.y >= this->y
              && inner.eright() <= this->eright()
              && inner.ebottom() <= this->ebottom());
    }

    constexpr bool operator==(Rect const & other) const noexcept
    {
        return (other.x == this->x
             && other.y == this->y
             && other.cx == this->cx
             && other.cy == this->cy);
    }

    constexpr bool operator!=(Rect const & other) const noexcept
    {
        return !(*this == other);
    }

    // Rect constructor ensures that any empty rect will be (0, 0, 0, 0)
    // hence testing cx or cy is enough
    constexpr bool isempty() const noexcept
    {
        return (this->cx == 0) || (this->cy == 0);
    }

    constexpr int getCenteredX() const noexcept
    {
        return this->x + (this->cx / 2);
    }

    constexpr int getCenteredY() const noexcept
    {
        return this->y + (this->cy / 2);
    }

    constexpr Dimension get_dimension() const noexcept
    {
        return Dimension(this->cx, this->cy);
    }

    // compute a new rect containing old rect and given point
    constexpr Rect enlarge_to(int16_t x, int16_t y) const noexcept
    {
        if (this->isempty()){
            return Rect(x, y, 1, 1);
        }

        const int16_t x0 = std::min(this->x, x);
        const int16_t y0 = std::min(this->y, y);
        const int16_t x1 = std::max(static_cast<int16_t>(this->eright() - 1), x);
        const int16_t y1 = std::max(static_cast<int16_t>(this->ebottom() - 1), y);
        return Rect(x0, y0, uint16_t(x1 - x0 + 1), uint16_t(y1 - y0 + 1));
    }

    constexpr Rect offset(int16_t dx, int16_t dy) const noexcept
    {
        return Rect(
            static_cast<int16_t>(this->x + dx),
            static_cast<int16_t>(this->y + dy),
            this->cx,
            this->cy
        );
    }

    constexpr Rect offset(Point point) const noexcept
    {
        return this->offset(point.x, point.y);
    }

    constexpr Rect shrink(uint16_t margin) const noexcept
    {
        assert((this->cx >= margin * 2) && (this->cy >= margin * 2));
        return Rect(
            static_cast<int16_t>(this->x + margin),
            static_cast<int16_t>(this->y + margin),
            static_cast<uint16_t>(this->cx - margin * 2),
            static_cast<uint16_t>(this->cy - margin * 2)
        );
    }

    constexpr Rect expand(uint16_t margin) const noexcept
    {
        return Rect(
            static_cast<int16_t>(this->x - margin),
            static_cast<int16_t>(this->y - margin),
            static_cast<uint16_t>(this->cx + margin * 2),
            static_cast<uint16_t>(this->cy + margin * 2)
        );
    }

    constexpr Rect intersect(uint16_t width, uint16_t height) const noexcept
    {
        return this->intersect(Rect(0, 0, width, height));
    }

    constexpr Rect intersect(Rect in) const noexcept
    {
        int16_t max_x = std::max(in.x, this->x);
        int16_t max_y = std::max(in.y, this->y);
        int16_t min_right = std::min(in.eright(), this->eright());
        int16_t min_bottom = std::min(in.ebottom(), this->ebottom());

        return Rect(max_x, max_y, uint16_t(min_right - max_x), uint16_t(min_bottom - max_y));
    }

    /// Intersection with a positive rect.
    /// Equivalent to self.intersect(Rect(0, 0, max, max)).
    constexpr Rect to_positive() const noexcept
    {
        Rect rect = *this;

        auto ux = static_cast<uint16_t>(-rect.x);
        auto uy = static_cast<uint16_t>(-rect.y);

        if ((rect.x < 0 && rect.cx < ux) || (rect.y < 0 && rect.cy < uy)) {
            return {
                rect.x < 0 ? int16_t{} : rect.x,
                rect.y < 0 ? int16_t{} : rect.y,
                0,
                0,
            };
        }

        if (rect.x < 0) {
            rect.x = 0;
            rect.cx -= ux;
        }

        if (rect.y < 0) {
            rect.y = 0;
            rect.cy -= uy;
        }

        return rect;
    }

    constexpr Rect disjunct(Rect r) const noexcept
    {
        if (this->isempty()) {
            return r;
        }
        if (r.isempty()) {
            return *this;
        }

        auto x = std::min(r.x, this->x);
        auto y = std::min(r.y, this->y);
        auto cx = std::max(r.eright(), this->eright()) - x;
        auto cy = std::max(r.ebottom(), this->ebottom()) - y;
        return Rect(x, y, uint16_t(cx), uint16_t(cy));
    }

    constexpr bool has_intersection(Rect in) const noexcept
    {
        return (this->cx && this->cy && !in.isempty()
        && ((in.x >= this->x && in.x < this->eright()) || (this->x >= in.x && this->x < in.eright()))
        && ((in.y >= this->y && in.y < this->ebottom()) || (this->y >= in.y && this->y < in.ebottom()))
        );
    }

    // Ensemblist difference
    template<class Fn>
    void difference(Rect a, Fn fn) const
    {
        const Rect intersect = this->intersect(a);

        if (!intersect.isempty()) {
            if (intersect.y > this->y) {
                fn(Rect(this->x, this->y,
                        this->cx, static_cast<uint16_t>(intersect.y - this->y)));
            }
            if (intersect.x > this->x) {
                fn(Rect(this->x, intersect.y,
                        static_cast<uint16_t>(intersect.x - this->x), intersect.cy));
            }
            if (this->eright() > intersect.eright()) {
                fn(Rect(intersect.eright(), intersect.y,
                        static_cast<uint16_t>(this->eright() - intersect.eright()), intersect.cy));
            }
            if (this->y + this->cy > intersect.ebottom()) {
                fn(Rect(this->x, intersect.ebottom(),
                        this->cx, static_cast<uint16_t>(this->ebottom() - intersect.ebottom())));
            }
        }
        else {
            fn(*this);
        }
    }

    REDEMPTION_FRIEND_OSTREAM(out, Rect const & r)
    {
        return out << "(" << r.x << ", " << r.y << ", " << r.cx << ", " << r.cy << ")";
    }
};

inline auto log_value(Rect const & rect) noexcept
{
    struct {
        char value[128];
    } d;
    ::sprintf(d.value, "Rect(%d %d %u %u)", rect.x, rect.y, rect.cx, rect.cy);
    return d;
}


// helper class used to compute differences between two rectangles
class DeltaRect {
    public:
    int dleft;
    int dtop;
    int dheight;
    int dwidth;

    constexpr DeltaRect(Rect r1, Rect r2) noexcept
    : dleft(r1.x - r2.x)
    , dtop(r1.y - r2.y)
    , dheight(r1.cy - r2.cy)
    , dwidth(r1.cx - r2.cx)
    {}

    constexpr bool fully_relative() const noexcept
    {
        return (this->dleft < 128 && this->dleft > -128)
            && (this->dtop < 128 && this->dtop > -128)
            && (this->dwidth < 128 && this->dwidth > -128)
            && (this->dheight < 128 && this->dheight > -128)
            ;
    }
};

inline auto log_value(DeltaRect const & delta) noexcept
{
    struct {
        char value[128];
    } d;
    ::sprintf(d.value, "DeltaRect(%d %d %d %d)",
        delta.dleft, delta.dtop, delta.dheight, delta.dwidth);
    return d;
}
