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
  Copyright (C) Wallix 2019
  Author(s): Christophe Grosjean

  Windows Execute Shell Parameters
*/

#pragma once

#include <string>
#include "utils/log.hpp"

struct WindowsExecuteShellParams {
    uint16_t    flags = 0;
    std::string exe_or_file;
    std::string working_dir;
    std::string arguments;

    void log(int level) const
    {
        LOG(level,
            "WindowsExecuteShellParams: Flags: %u exe_or_file: %s working_dir: %s arguments: %s",
            flags,
            this->exe_or_file,
            this->working_dir,
            this->arguments);
    }
};

