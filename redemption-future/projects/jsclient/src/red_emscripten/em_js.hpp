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
Copyright (C) Wallix 2010-2018
Author(s): Jonathan Poelen
*/

#pragma once

#ifdef IN_IDE_PARSER
# define RED_EM_JS(return_type, name, params, ...) return_type name params;
# define RED_EM_ASYNC_JS(return_type, name, params, ...) return_type name params;
#else
# include <emscripten/emscripten.h> /* fix with 1.39.16 */
# include <emscripten/em_js.h>
# include "cxx/diagnostic.hpp"
# define RED_EM_JS(return_type, name, params, ...)                 \
    REDEMPTION_DIAGNOSTIC_PUSH()                                   \
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wmissing-prototypes")     \
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wunknown-warning-option") \
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wreserved-identifier")    \
    EM_JS(return_type, name, params, __VA_ARGS__)                  \
    REDEMPTION_DIAGNOSTIC_POP()
# define RED_EM_ASYNC_JS(return_type, name, params, ...)           \
    REDEMPTION_DIAGNOSTIC_PUSH()                                   \
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wmissing-prototypes")     \
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wunknown-warning-option") \
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wreserved-identifier")    \
    EM_ASYNC_JS(return_type, name, params, __VA_ARGS__)            \
    REDEMPTION_DIAGNOSTIC_POP()
#endif
