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

   Unit test to rect object

*/

#include "test_only/test_framework/redemption_unit_tests.hpp"


#include "utils/rect.hpp"


RED_AUTO_TEST_CASE(TestRect)
{
    // A rect is defined by it's top left corner, width and height
    Rect r(10, 110, 10, 10);

    RED_CHECK_EQUAL(10, r.x);
    RED_CHECK_EQUAL(110, r.y);
    RED_CHECK_EQUAL(20, r.eright());
    RED_CHECK_EQUAL(120, r.ebottom());

    /* we can also create an empty rect, it's the default constructor */
    Rect empty;

    RED_CHECK_EQUAL(0, empty.x);
    RED_CHECK_EQUAL(0, empty.y);
    RED_CHECK_EQUAL(0, empty.eright());
    RED_CHECK_EQUAL(0, empty.ebottom());

    /* test if rect is empty */
    RED_CHECK(empty.isempty());
    RED_CHECK(not r.isempty());

    /* this one is empty because left is after right */
    Rect e1(20, 110, -10, 10);
    RED_CHECK(e1.isempty());


    Rect e2(10, 110, 0, 0);
    RED_CHECK(e2.isempty());

    Rect e3(10, 110, 10, 0);
    RED_CHECK(e3.isempty());

    Rect e4(10, 110, 10, 1);
    RED_CHECK(not e4.isempty());
    RED_CHECK_EQUAL(Rect(-10, -20, 10, 1), e4.offset(-20, -130));
    RED_CHECK_EQUAL(Rect(-10, -20, 10, 1), Rect(10, 110, 10, 1).offset(-20, -130));

    /* test if a point is inside rect */
    /* lower bounds are include ", upper bounds are excluded */

    RED_CHECK(r.contains_pt(15,115));
    RED_CHECK(r.contains_pt(19,119));
    RED_CHECK(r.contains_pt(10,110));
    RED_CHECK(r.contains_pt(10,119));

    RED_CHECK(not r.contains_pt(0,100));
    RED_CHECK(not r.contains_pt(0,115));
    RED_CHECK(not r.contains_pt(15,100));
    RED_CHECK(not r.contains_pt(15,121));
    RED_CHECK(not r.contains_pt(21,115));
    RED_CHECK(not r.contains_pt(20,120));
    RED_CHECK(not r.contains_pt(19,120));

    RED_CHECK(Rect().to_positive() == Rect());
    RED_CHECK(Rect(10, 20, 5, 40).to_positive() == Rect(10, 20, 5, 40));
    RED_CHECK(Rect(10, -20, 5, 40).to_positive() == Rect(10, 0, 5, 20));
    RED_CHECK(Rect(-10, 20, 15, 40).to_positive() == Rect(0, 20, 5, 40));
    RED_CHECK(Rect(-10, 20, 5, 40).to_positive() == Rect(0, 20, 0, 0));
    RED_CHECK(Rect(20, -10, 40, 5).to_positive() == Rect(20, 0, 0, 0));

    auto intersect = [](Rect a, Rect b) { return a.intersect(b); };

    /* we can build the intersection of two rect */
    RED_CHECK(intersect(
            {10, 110, 30, 30},
            {20, 120, 10, 10}
        ) == Rect(
            20, 120, 10, 10
        ));
    RED_CHECK(intersect(
            {10, 110, 20, 20},
            {20, 120, 20, 20}
        ) == Rect(
            20, 120, 10, 10
        ));
    /* This one is empty, it could yield any empty rect */
    RED_CHECK(intersect(
            {10, 110, 10, 10},
            {20, 110, 20, 10}
        ) == Rect(
            20, 110, 0, 0
        ));
    RED_CHECK(intersect(
            {10, 15, 20, 20},
            {100, 90, 10, 30}
        ) == Rect(
            100, 90, 0, 0
        ));
    RED_CHECK(intersect(
            {-10, -20, 110, 120},
            {-5, -7, 155, 157}
        ) == Rect(
            -5, -7, 105, 107
        ));

    auto disjunct = [](Rect a, Rect b) { return a.disjunct(b); };

    /* we can build the union of two rect */
    /* here i2 is include " in i1 : then it is the intersection */
    RED_CHECK(disjunct(
            {10, 110, 30, 30},
            {20, 120, 10, 10}
        ) == Rect(
            10, 110, 30, 30
        ));
    RED_CHECK(disjunct(
            {10, 110, 20, 20},
            {20, 120, 20, 20}
        ) == Rect(
            10, 110, 30, 30
        ));
    RED_CHECK(disjunct(
            {10, 110, 10, 10},
            {10, 110, 10, 10}
        ) == Rect(
            10, 110, 10, 10
        ));
    /* here i2 is include " in i1 : then it is the intersection */
    RED_CHECK(disjunct(
            {-10, -20, 110, 120},
            {-5, -7, 155, 157}
        ) == Rect(
            -10, -20, 160, 170
        ));

    /* we can move a rect by some offset */
    RED_CHECK(Rect(10, 110, 10, 10).offset(10, 100) == Rect(20, 210, 10, 10));

    {
        /* check if a rectangle contains another */
        Rect r(10, 10, 10, 10);
        Rect inner(15, 15, 3, 3);

        RED_CHECK(r.contains(inner));
        RED_CHECK(r.contains(r));

        Rect bad1(9, 10, 10, 10);
        Rect good1(11, 10, 9, 10);
        RED_CHECK(not r.contains(bad1));
        RED_CHECK(r.contains(good1));

        Rect bad2(10, 9, 10, 10);
        Rect good2(10, 11, 10, 9);
        RED_CHECK(not r.contains(bad2));
        RED_CHECK(r.contains(good2));

        Rect bad3(10, 10, 11, 10);
        Rect good3(10, 10, 9, 10);
        RED_CHECK(not r.contains(bad3));
        RED_CHECK(r.contains(good3));

        Rect bad4(10, 10, 10, 11);
        Rect good4(10, 10, 10, 9);
        RED_CHECK(not r.contains(bad4));
        RED_CHECK(r.contains(good4));
    }

    {
        /* check if two rectangles are identicals */
        Rect r(10, 10, 10, 10);

        Rect same(10, 10, 10, 10);
        Rect inner(15, 15, 3, 3);
        Rect outer(1, 1, 30, 30);
        Rect bad1(9, 10, 10, 10);
        Rect bad2(11, 10, 9, 10);
        Rect bad3(10, 9, 10, 10);
        Rect bad4(10, 11, 10, 9);
        Rect bad5(10, 10, 11, 10);
        Rect bad6(10, 10, 9, 10);
        Rect bad7(10, 10, 10, 11);
        Rect bad8(10, 10, 10, 9);

        RED_CHECK_EQUAL(r, r);
        RED_CHECK_EQUAL(r, same);
        RED_CHECK_EQUAL(same, r);
        RED_CHECK_NE(r, inner);
        RED_CHECK_NE(r, outer);
        RED_CHECK_NE(r, bad1);
        RED_CHECK_NE(r, bad2);
        RED_CHECK_NE(r, bad3);
        RED_CHECK_NE(r, bad4);
        RED_CHECK_NE(r, bad5);
        RED_CHECK_NE(r, bad6);
        RED_CHECK_NE(r, bad7);
        RED_CHECK_NE(r, bad8);
    }

    // for RDP relative sending we often need to have difference of coordinates
    // between two rectangles. That is what DeltaRect is made for
    // Notice it uses top/left/right/bottom instead of x/x/cx/cy to clearly
    // differentiate bothe types of rects.

    // 222222222222222
    // 2             2
    // 2             2
    // 2             2
    // 2             2
    // 2             2
    // 2             2
    // 2             2
    // 2             2
    // 2             2
    // 2         11112111111
    // 2         1   2     1
    // 2         1   2     1
    // 2         1   2     1
    // 222222222212222     1
    //           1         1
    //           1         1
    //           1         1
    //           1         1
    //           11111111111
    //


    Rect r1(10, 15, 11, 10);
    Rect r2(0, 0, 15, 20);
    DeltaRect dr(r1, r2);
    RED_CHECK_EQUAL(10, dr.dleft);
    RED_CHECK_EQUAL(15, dr.dtop);
    RED_CHECK_EQUAL(-4, dr.dwidth);
    RED_CHECK_EQUAL(-10, dr.dheight);
    RED_CHECK(dr.fully_relative());

    DeltaRect dr2(r2, r1);
    RED_CHECK_EQUAL(-10, dr2.dleft);
    RED_CHECK_EQUAL(-15, dr2.dtop);
    RED_CHECK_EQUAL(4, dr2.dwidth);
    RED_CHECK_EQUAL(10, dr2.dheight);
    RED_CHECK(dr2.fully_relative());

    dr2.dheight = 1024;
    RED_CHECK(not dr2.fully_relative());

    {
        /* Test difference */
        Rect a(10, 10, 10, 10);
        Rect b(21, 21, 11, 11);

        a.difference(b, [](const Rect & b) {
            RED_CHECK_EQUAL(b, Rect(10, 10, 10, 10));
        });
    }

    {
        Rect a(10, 10, 50, 50);
        Rect b(20, 20, 10, 5);

        int counter = 0;
        a.difference(b, [&counter](const Rect & b) {
            switch(counter) {
                case 0:
                    RED_CHECK(b == Rect(10, 10, 50, 10));
                break;
                case 1:
                    RED_CHECK(b == Rect(10, 20, 10, 5));
                break;
                case 2:
                    RED_CHECK(b == Rect(30, 20, 30, 5));
                break;
                case 3:
                    RED_CHECK(b == Rect(10, 25, 50, 35));
                break;
                default:
                    RED_FAIL(counter);
            }
            ++counter;
        });
    }

    RED_CHECK_EQUAL(Rect(10, 10, 1, 1), Rect().enlarge_to(10, 10));
    RED_CHECK_EQUAL(Rect(200, 145, 1, 1054), Rect(200, 1198, 1, 1).enlarge_to(200, 145));
    RED_CHECK_EQUAL(Rect(145, 200, 1054, 1), Rect(1198, 200, 1, 1).enlarge_to(145, 200));
    RED_CHECK_EQUAL(Rect(10, 10, 91, 91), Rect(10, 10, 1, 1).enlarge_to(100, 100));
    RED_CHECK_EQUAL(Rect(10, 10, 91, 91), Rect(100, 100, 1, 1).enlarge_to(10, 10));

    RED_CHECK_EQUAL(Rect(10, 10, 20, 45).getCenteredX(), 20);

    RED_CHECK_EQUAL(Rect(10, 10, 100, 50).getCenteredY(), 35);


    RED_CHECK_EQUAL(Dimension(20, 45), Rect(45, 57, 20, 45).get_dimension());

    RED_CHECK_EQUAL(Rect(20, 30, 50, 80).shrink(15), Rect(35, 45, 20, 50));

    {
        Rect r(10, 110, 10, 10);
//         RED_CHECK_EQUAL(r.upper_side(), Rect(10, 110, 10, 1));
//         RED_CHECK_EQUAL(r.left_side(), Rect(10, 110, 1, 10));
//         RED_CHECK_EQUAL(r.lower_side(), Rect(10, 119, 10, 1));
//         RED_CHECK_EQUAL(r.right_side(), Rect(19, 110, 1, 10));

        RED_CHECK_EQUAL(r.intersect(15,115), Rect(10, 110, 5, 5));
    }

    {
        Rect r(10,10,10,10);
        RED_CHECK(not r.has_intersection(Rect(100,10,10,10)));
        RED_CHECK(not r.has_intersection(Rect(10,100,10,10)));
        RED_CHECK(not r.has_intersection(Rect(100,100,10,10)));
        RED_CHECK(not r.has_intersection(Rect(10,20,5,5)));
        RED_CHECK(not r.has_intersection(Rect(20,10,5,5)));
        RED_CHECK(not r.has_intersection(Rect(0,10,5,5)));
        RED_CHECK(not r.has_intersection(Rect(10,0,5,5)));
        RED_CHECK(not r.has_intersection(Rect(0,0,5,5)));
        RED_CHECK(not r.has_intersection(5,5));

        RED_CHECK(r.has_intersection(Rect(10,10,10,10)));
        RED_CHECK(r.has_intersection(Rect(5,10,10,10)));
        RED_CHECK(r.has_intersection(Rect(10,5,10,10)));
        RED_CHECK(r.has_intersection(Rect(15,10,10,10)));
        RED_CHECK(r.has_intersection(Rect(10,15,10,10)));
        RED_CHECK(r.has_intersection(Rect(15,15,5,5)));
        RED_CHECK(r.has_intersection(Rect(0,0,40,40)));
        RED_CHECK(r.has_intersection(15, 15));
    }
}

RED_AUTO_TEST_CASE(TestRect2)
{
    Rect r1(10, 10, 20, 20);
    Rect r2(15, 15, 0, 0);

    RED_CHECK(not r1.has_intersection(r2));
    RED_CHECK(not r2.has_intersection(r1));

    RED_CHECK_EQUAL(Rect(15, 15, 0, 0), r1.intersect(r2));
    RED_CHECK_EQUAL(Rect(15, 15, 0, 0), r2.intersect(r1));
}
