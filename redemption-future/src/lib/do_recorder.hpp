/*
   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Raphael Zhou

   redver video verifier program
*/

#pragma once

#include "capture/cryptofile.hpp"
#include "cxx/cxx.hpp"

extern "C" {
    REDEMPTION_LIB_EXPORT
    int do_main(int argc, char const ** argv,
            uint8_t const * hmac_key,
            get_trace_key_prototype * trace_fn);
}
