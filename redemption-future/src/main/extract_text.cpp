#include <mln/image/image2d.hh>

#include <iostream>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <string_view>

#include "capture/ocr/bitmap_as_ocr_image.hpp"
#include "capture/ocr/extract_bars.hh"
#include "capture/ocr/io_char_box.hpp"
#include "capture/ocr/extract_text_classification.hh"
#include "utils/sugar/chars_to_int.hpp"

using namespace std::string_view_literals;

struct Classification
{
    std::vector<ocr::label_attr_t> attrs;
    std::vector<ocr::classifier_type> results;
    ocr::classifier_type classifier;
    mln::image2d<bool> ima;
    ocr::fonts::LocaleId locale_id = ocr::fonts::LocaleId::latin;
    unsigned font_id;
    bool display_char;

    Classification(unsigned font_id_, bool display_char_)
    : font_id(font_id_)
    , display_char(display_char_)
    {}

    void operator()(ocr::BitmapAsOcrImage const & input, unsigned tid, ppocr::Box const & box, unsigned button_col)
    {
        ocr::image_view_to_image2d_bool(input, tid, this->ima, box);

        this->attrs.clear();
        ocr::labelize(this->attrs, this->ima);

        if (this->display_char) {
            display_char_box(std::cout, this->ima, this->attrs);
        }

        if (font_id == -1u) {
            const unsigned nfonts = ocr::fonts::nfonts[static_cast<unsigned>(this->locale_id)];
            this->results.resize(nfonts);
            unsigned normally_selected_id = -1u;
            bool is_selected = false;
            unsigned unrecognized_rate = 100;
            for (unsigned id = 0; id < nfonts; ++id) {
                ocr::classifier_type & res = this->results[id];
                res.classify(this->attrs, this->ima, this->locale_id, id);
                const unsigned unrecognized_rate2 = res.unrecognized_rate();
                if (!is_selected && unrecognized_rate2 < unrecognized_rate) {
                    normally_selected_id = id;
                    unrecognized_rate = unrecognized_rate2;
                    if (res.is_recognize()) {
                        is_selected = true;
                    }
                }
            }

            std::cout << "-------- font: " << (
                normally_selected_id == - 1u
                ? "(none)"
                : ocr::fonts::fonts[static_cast<unsigned>(this->locale_id)][normally_selected_id].name
            ) << "  " << PrintableBox{box} << " --------\n";
            if (normally_selected_id == -1u) {
                normally_selected_id = unsigned(this->results.size() - 1);
            }
            typedef std::vector<ocr::classifier_type>::iterator iterator;
            iterator first = this->results.begin();
            for (std::size_t i = 0; i < this->results.size(); ++i, ++first) {
                std::cout << (i == normally_selected_id ? "[*] " : "[ ] ");
                display_result(input, tid, i, box, button_col, *first);
            }
        }
        else {
            std::cout << "-------- font: "
              << ocr::fonts::fonts[static_cast<unsigned>(this->locale_id)][font_id].name
              << "  " << PrintableBox{box} << " --------\n"
            ;
            this->classifier.classify(this->attrs, this->ima, this->locale_id, this->font_id);
            display_result(input, tid, this->font_id, box, button_col, this->classifier);
        }
    }

private:
    void display_result(
        ocr::BitmapAsOcrImage const & input, unsigned tid, unsigned font_id,
        ppocr::Box const & box, unsigned button_col, const ocr::classifier_type & res)
    {
        auto id = static_cast<unsigned>(this->locale_id);
        const bool is_title_bar = ocr::is_title_bar(
            input, tid, box.y(), box.bottom(), button_col
          , ocr::fonts::fonts[id][res.font_id].max_height_char
        );
        std::cout
            << (&"other\0title"[is_title_bar*6])
            << ": " << res.out
            << "\n - locale: " << &"latin\0cyrillic"[id * 6]
            << "\n - font: " << ocr::fonts::fonts[id][font_id].name
            << "\n - length: " << res.character_count
            << "  unrecognized: " << res.unrecognized_count
            << "  unrecognized rate: " << res.unrecognized_rate()
            << "%  first unrecognized index: " << res.first_unrecognized_index
            << "\n"
        ;
    }
};

struct ReferenceClassification
{
    Classification & ref;

    ReferenceClassification(Classification & ref_)
    : ref(ref_)
    {}

    void operator()(ocr::BitmapAsOcrImage const & input, unsigned tid, ppocr::Box const & box, unsigned button_col)
    {
        this->ref(input, tid, box, button_col);
    }
};


inline void usage(char** argv)
{
    std::cerr << "Usage: " << argv[0] << " input.png [latin|cyrillic] [font_id|font_name] [d]\n";
    const auto max = static_cast<unsigned>(ocr::fonts::LocaleId::max);
    for (unsigned locale_id = 0; locale_id < max; ++locale_id) {
        std::cout << "\nlocale (" << locale_id << "): " << &"latin\0cyrillic"[locale_id * 6] << "\n";
        for (unsigned i = 0; i < ocr::fonts::nfonts[locale_id]; ++i) {
            std::cerr << "  " << i << " - " << ocr::fonts::fonts[locale_id][i].name << "\n";
        }
        std::cerr << " -1 - all\n";
    }
    std::cerr << "\n  d - display characters" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 2 || argc > 5) {
        usage(argv);
        return 1;
    }

    ocr::fonts::LocaleId locale_id = ocr::fonts::LocaleId::latin;
    unsigned font_id = -1u;
    bool display_char = false;
    if (argc == 3 || argc == 4 || argc == 5) {
        int iarg = 2;
        std::string_view arg = argv[iarg];

        if (arg == "latin"sv) {
            locale_id = ocr::fonts::LocaleId::latin;
            ++iarg;
        }
        else if (arg == "cyrillic"sv) {
            locale_id = ocr::fonts::LocaleId::cyrillic;
            ++iarg;
        }

        if (iarg < argc) {
            arg = argv[iarg];
            if (arg == "d"sv) {
                display_char = true;
            }
            else {
                if (arg != "all"sv && arg != "-1"sv) {
                    const auto n = parse_decimal_chars<int>(arg);
                    font_id = n.has_value
                        ? unsigned(n.value)
                        : ocr::fonts::font_id_by_name(locale_id, arg);
                    if (font_id == -1u || font_id >= ocr::fonts::nfonts[static_cast<unsigned>(locale_id)]) {
                        std::cerr << "error: invalid font_id\n";
                        usage(argv);
                        return 2;
                    }
                }

                if (++iarg < argc) {
                    if (!('d' == argv[iarg][0] && !argv[iarg][1])) {
                        usage(argv);
                        return 3;
                    }
                    display_char = true;
                }
            }
        }
    }

    ocr::BitmapAsOcrImage input(argv[1]);
    if (!input.is_valid()) {
        if (errno != 0) {
            std::cerr << argv[0] << ": " << strerror(errno) << std::endl;
        }
        return 4;
    }

    ocr::ExtractTitles extract_titles;
    if (font_id != -1u) {
        auto& font_info = ::ocr::fonts::fonts[static_cast<unsigned>(locale_id)][font_id];
        extract_titles.set_box_height(font_info.min_height_char, font_info.max_height_char);
    }

    using resolution_clock = std::chrono::high_resolution_clock;
    auto t1 = resolution_clock::now();
    Classification classifiaction(font_id, display_char);
    extract_titles.extract_titles(input, ReferenceClassification(classifiaction));
    auto t2 = resolution_clock::now();
    std::cerr << '\n' << std::chrono::duration<double>(t2-t1).count() << "s\n";

    return 0;
}
