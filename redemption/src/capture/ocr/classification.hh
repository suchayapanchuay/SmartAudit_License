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
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen, Meng Tan
 */

#pragma once

#include <mln/image/image2d.hh>

#include "fonts.hpp"
#include "labelize.hh"
#include "extract_data.hh"
#include "capture/ocr/locale/latin_to_cyrillic.hpp"

#include <vector>
#include <string>

namespace ocr {

struct classifier_type
: private locale::latin_to_cyrillic_context
{
    std::string out;

private:
    std::string out_tmp;

public:
    unsigned character_count{0};
    unsigned unrecognized_count{0};
    unsigned first_unrecognized_index{0};
    unsigned font_id;

    explicit classifier_type(unsigned font = -1u) /*NOLINT*/
    : font_id(font)
    {}

    [[nodiscard]] unsigned unrecognized_rate() const
    {
        if (this->character_count) {
            return this->unrecognized_count * 100 / this->character_count;
        }
        return 100;
    }

    void clear()
    {
        this->out.clear();
        this->character_count = 0;
        this->unrecognized_count = 0;
        this->first_unrecognized_index = 0;
        this->font_id = 0;
    }

    void classify(
        std::vector<ocr::label_attr_t> attrs, const ::mln::image2d<bool> & input
      , ocr::fonts::LocaleId local_id, unsigned font_id
    ) {
        this->clear();
        this->font_id = font_id;
        this->out.reserve(attrs.size());
        bool beginning = true;
        auto first = attrs.begin();
        auto last = attrs.end();

        if (first == last) {
            return ;
        }

        const fonts::Font & font = fonts::fonts[static_cast<unsigned>(local_id)][font_id];

        for (; first < last; ++first) {
            const ::ppocr::Box & bbox = first->bbox;
            std::string_view c = font.classify(bbox.width(), bbox.height(), first->area, fonts::Pixel{input, bbox, bbox.width()});

            if (!beginning) {
                if (first->bbox.x() - (first-1)->bbox.right() >= int(font.whitespace_width)) {
                    this->out += ' ';
                }
                this->out += c;
                ++this->character_count;

                if (c.data() == unknown) {
                    ++this->unrecognized_count;
                }
            }
            else if (c.data() == unknown) {
                ++this->first_unrecognized_index;
            }
            else {
                beginning = false;
                this->out += c;
                ++this->character_count;
            }
        }

        if (font.replacements.data()) {
            for (fonts::Font::Replacement const& replacement : font.replacements) {
                std::string::size_type pos = 0;
                // TODO free function
                while (pos != std::string::npos) {
                    const auto fpos = this->out.find(replacement.pattern.data(), pos, replacement.pattern.size());
                    if (fpos != std::string::npos) {
                        this->out.replace(fpos, replacement.pattern.size(), replacement.replace);
                    }
                    pos = fpos;
                }
            }
        }

        if (local_id == ocr::fonts::LocaleId::cyrillic) {
            this->latin_to_cyrillic(this->out);
        }
    }

    [[nodiscard]] inline bool is_recognize() const
    {
        return 0 == this->unrecognized_count && this->first_unrecognized_index < 7 && this->character_count > 5;
    }
};


struct Classification
{
    using result_type = classifier_type;

    Classification() = default;

    const classifier_type & classify(
        const ::mln::image2d<bool> & input
      , ocr::fonts::LocaleId local_id = ocr::fonts::LocaleId::latin /*NOLINT*/
      , unsigned font_id = -1u /*NOLINT*/
    ) {
        this->attrs.clear();
        labelize(this->attrs, input);

        if (font_id == -1u) {
            this->classifier.clear();
            classifier_type classify_info;
            unsigned unrecognized_rate = 100;
            const unsigned nfonts = fonts::nfonts[static_cast<unsigned>(local_id)];
            for (unsigned id = 0; id < nfonts; ++id) {
                classify_info.classify(this->attrs, input, local_id, id);
                const unsigned unrecognized_rate2 = classify_info.unrecognized_rate();
                if (unrecognized_rate2 < unrecognized_rate) {
                    this->classifier = classify_info;
                    unrecognized_rate = unrecognized_rate2;
                    if (this->classifier.is_recognize()) {
                        break;
                    }
                }
            }
        }
        else {
            this->classifier.classify(this->attrs, input, local_id, font_id);
        }
        return this->classifier;
    }

    Classification(const Classification &) = delete;
    Classification & operator=(const Classification &) = delete;

private:
    std::vector<label_attr_t> attrs;
    classifier_type classifier;
};

} // namespace ocr
