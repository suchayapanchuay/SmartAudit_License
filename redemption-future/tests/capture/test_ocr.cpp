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
   Copyright (C) Wallix 2010-2012
   Author(s): Jonathan Poelen

   Unit test to detect memory leak in OCR module
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "capture/title_extractors/ocr_titles_extractor.hpp"
#include "capture/title_extractors/ocr_title_filter.hpp"

#include "utils/drawable.hpp"
#include "utils/bitmap_from_file.hpp"

namespace {
    void draw_bitmap(Drawable & drawable, char const * bitmap_filename) {
        Bitmap bmp = bitmap_from_file_impl(bitmap_filename, NamedBGRColor::BLACK);
        RED_REQUIRE(bmp.is_valid());
        drawable.draw_bitmap({0, 0, drawable.width(), drawable.height()}, bmp);
    }
} // namespace

RED_AUTO_TEST_CASE(TestNewOCR1)
{
    Drawable drawable(800, 600);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    {
        draw_bitmap(drawable, FIXTURES_PATH "/m-21288-2.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "Gestionnaire de serveur"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/w7mediacentercapture.png");
        extractor.extract_titles(drawable, out_titles);

        RED_CHECK_EQUAL(out_titles.size(), 0);
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008capture2.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "licences TSGestionnaire de licences TS"_av,
            "Gestionnaire de licences TSGestionnaire de licences TS"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 1);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008capture4.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "plop fi test fififid zjhufh .txt - Bloc-notes"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008capture.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "Gestionnaire de licences TS"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008capture3.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "Gestionnaire de serveur"_av,
            "Gestionna? r de ? ?en? s TS"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008capture5.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "ff fff ffff f fa fz fe fr ft fy fu fi fo fp fq fs fd fg fh fj fk fl fm fw fx fc fv fb fn f, f;"
            " f% fè fé f& f# fà f@ ffi p.txt - Bloc-notes"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    // extended ligatures ffff etc are unsupported yet
    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008capture6.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "ff fff ffff ? ? ? pp ff p fff p ffff p ? p ? p ? p.txt - Bloc-notes"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008capture9.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "af rt rf th jk kk ok oo lo LM .txt - Bloc-notes"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008capture10.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "qwhybcaliueLkaASsFkkUibnkzkwwkswq.txt - Bloc-notes"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }
}

RED_AUTO_TEST_CASE(Test2008R2)
{
    Drawable drawable(653, 244);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008R2_server_manager.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "Server Manager"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());
    }
}

RED_AUTO_TEST_CASE(TestNewOCR2)
{
    Drawable drawable(800, 600);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    {
        extractor.extract_titles(drawable, out_titles);
        RED_CHECK_EQUAL(out_titles.size(), 0);
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2008capture10.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "qwhybcaliueLkaASsFkkUibnkzkwwkswq.txt - Bloc-notes"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    RED_CHECK_EQUAL(
        extractor.selected_font_id(), ocr::fonts::font_id_by_name(LocaleId::latin, "tahoma_8_bold")
    );

    {
        draw_bitmap(drawable, FIXTURES_PATH "/"
                              "rdp@192.168.10.254,qaadministrateur@win2k3dc_rdp,20121128-114532,"
                              "wab2-3-1-0.yourdomain,8575-001616-000000.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            ".es"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }
}

RED_AUTO_TEST_CASE(TestNewOCR3)
{
    Drawable drawable(800, 600);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2002capture1.png");
        extractor.set_font_id(ocr::fonts::font_id_by_name(ocr::fonts::LocaleId::latin, "tahoma_8_bold"));
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "toto"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }
}

RED_AUTO_TEST_CASE(TestNewOCR4)
{
    Drawable drawable(800, 600);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    {
        draw_bitmap(drawable, FIXTURES_PATH "/capture1.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "É.txt - Bloc-notes"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/capture2.png");
        extractor.set_font_id(-1);
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "Poste de travail"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }
}

RED_AUTO_TEST_CASE(TestNewOCRWin2012)
{
    Drawable drawable(1446, 927);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2012capture2.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "Hyper-V Manager"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2012capture3.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "wab_hv-12"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }
}

RED_AUTO_TEST_CASE(TestNewOCRWin2012_VNC)
{
    Drawable drawable(1023, 768);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    {
        draw_bitmap(drawable, FIXTURES_PATH "/win2012_vnc_t9209.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "Bibliothèques"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }
}

RED_AUTO_TEST_CASE(TestNewOCRWin2012Standard)
{
    Drawable drawable(2880, 900);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    {
        draw_bitmap(drawable, FIXTURES_PATH "/w2012s_Gestionnaire_de_serveur.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
                "Gestionnaire de serveur"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/w2012s_Gestionnaire_des_taches.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
                "Gestionnaire des tàches"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }

    {
        draw_bitmap(drawable, FIXTURES_PATH "/w2012s_Panneau_de_configuration.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
                "Panneau de configuration"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());

        out_titles.clear();
    }
}


RED_AUTO_TEST_CASE(TestOCRBug)
{
    Drawable drawable(1364, 768);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    {
        draw_bitmap(drawable, FIXTURES_PATH "/badocr.png");
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "Computer"_av
        };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());
    }
}


RED_AUTO_TEST_CASE(TestNewOCRRussian)
{
    RED_TEST_DATAS(
        (FIXTURES_PATH "/win_unknown_russian.png")
        (FIXTURES_PATH "/win_unknown_russian2.png")
    ) >>= [&](char const* filename)
    {
        Drawable drawable(816, 639);

        using ocr::locale::LocaleId;
        OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::cyrillic});

        OcrTitleFilter filter;
        std::vector<OcrTitle> out_titles;

        draw_bitmap(drawable, filename);
        extractor.extract_titles(drawable, out_titles);

        std::array expected{
            "Устройства и принтеры"_av
            };
        RED_CHECK_EQUAL(out_titles.size(), expected.size());

        auto idx_best = filter.extract_best_title(out_titles);
        RED_CHECK_EQUAL(idx_best, 0);
        RED_CHECK(expected[idx_best] == filter.get_title());
    };
}

RED_AUTO_TEST_CASE(TestBogusOCR)
{
    Drawable drawable(800, 600);

    using ocr::locale::LocaleId;
    OcrTitlesExtractor extractor(true, 100, LocaleId{LocaleId::latin});

    OcrTitleFilter filter;
    std::vector<OcrTitle> out_titles;

    draw_bitmap(drawable, FIXTURES_PATH "/win2012capture1.png");
    extractor.extract_titles(drawable, out_titles);

    auto expected = "a b c d e f g h ij k l m n o p q r s t u v w xyz - Bloc-notes"_av;
    RED_CHECK(out_titles.size() == 1);

    auto idx_best = filter.extract_best_title(out_titles);
    RED_CHECK(idx_best == 0);
    RED_CHECK(expected == filter.get_title());
}
