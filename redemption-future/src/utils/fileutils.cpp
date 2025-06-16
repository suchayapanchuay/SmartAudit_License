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
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Raphael Zhou

   File related utility functions
*/

#include "utils/fileutils.hpp"
#include "utils/log.hpp"
#include "utils/file.hpp"
#include "utils/strutils.hpp"
#include "utils/sugar/unique_fd.hpp"

#include <cstdio>
#include <cstddef>
#include <cerrno>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <alloca.h>

#ifdef __EMSCRIPTEN__
char const* basename(char const* path) noexcept
{
    char const* p = path;

    do {
        while (*p != '/' && *p) {
            ++p;
        }

        if (!*p) {
            return path;
        }

        ++p;
        path = p;
    } while (*path);

    return path;
}

char* basename(char* path) noexcept
{
    return const_cast<char*>(basename(const_cast<char const*>(path)));
}
#endif

// two flavors of basename_len to make it const agnostic
const char * basename_len(const char * path, size_t & len) noexcept
{
    const char * tmp = strrchr(path, '/');
    if (tmp) {
        path = tmp + 1;
    }
    len = strlen(path);
    return path;
}

char * basename_len(char * path, size_t & len) noexcept /* NOLINT(readability-non-const-parameter) */
{
    char const * const_path = path;
    return const_cast<char*>(basename_len(const_path, len)); /*NOLINT*/
}

int64_t filesize(const char * path) noexcept
{
    struct stat sb;
    int status = stat(path, &sb);
    if (status >= 0){
        return sb.st_size;
    }
    //LOG(LOG_INFO, "%s", strerror(errno));
    return -1;
}

bool file_exist(const char * path) noexcept
{
    struct stat sb;
    return (stat(path, &sb) == 0);
}

bool dir_exist(const char * path) noexcept
{
    struct stat sb;
    int statok = ::stat(path, &sb);
    return (statok == 0) && ((sb.st_mode & S_IFDIR) != 0);
}


int64_t filesize(std::string const& path) noexcept
{
    return filesize(path.c_str());
}

bool file_exist(std::string const& path) noexcept
{
    return file_exist(path.c_str());
}

bool dir_exist(std::string const& path) noexcept
{
    return dir_exist(path.c_str());
}


SplitedPath ParsePath(const std::string & fullpath)
{
    SplitedPath result;
    const char * end_of_directory = strrchr(fullpath.c_str(), '/');
    if (end_of_directory > fullpath.c_str()) {
        result.directory.assign(fullpath.c_str(), end_of_directory - fullpath.c_str() + 1);
    }

    const char * begin_of_filename =
        (end_of_directory ? end_of_directory + 1 : fullpath.c_str());
    const char * end_of_filename   =
        [&] () {
            const char * dot = strrchr(begin_of_filename, '.');
            if (!dot || (dot == begin_of_filename)) {
                return fullpath.c_str() + fullpath.size();
            }
            return dot;
        } ();

    result.basename.assign(begin_of_filename, end_of_filename);

    if (*end_of_filename) {
        result.extension = end_of_filename;
    }
    return result;
}


void MakePath(std::string & fullpath, const char * directory,
              const char * filename, const char * extension)
{
    fullpath = (directory ? directory : "");
    if (!fullpath.empty() && (fullpath.back() != '/')) { fullpath += '/'; }
    if (filename) { fullpath += filename; }
    if (extension && *extension) {
        if (*extension != '.') { fullpath += '.'; }
        fullpath += extension;
    }
}

bool canonical_path(const char * fullpath, char * path, size_t path_len,
                    char * basename, size_t basename_len, char * extension,
                    size_t extension_len)
{
    const char * end_of_path = strrchr(fullpath, '/');
    if (end_of_path){
        if (static_cast<size_t>(end_of_path + 1 - fullpath) <= path_len) {
            memcpy(path, fullpath, end_of_path + 1 - fullpath);
            path[end_of_path + 1 - fullpath] = 0;
        }
        else {
            LOG(LOG_ERR, "canonical_path : Path too long for the buffer");
            return false;
        }
        const char * start_of_extension = strrchr(end_of_path + 1, '.');
        if (start_of_extension){
            utils::strlcpy(extension, start_of_extension, extension_len);
            if (start_of_extension > end_of_path + 1){
                if (static_cast<size_t>(start_of_extension - end_of_path - 1) <= basename_len) {
                    memcpy(basename, end_of_path + 1, start_of_extension - end_of_path - 1);
                    basename[start_of_extension - end_of_path - 1] = 0;
                }
                else {
                    LOG(LOG_ERR, "canonical_path : basename too long for the buffer");
                    return false;
                }
            }
            // else no basename : leave output buffer for name untouched
        }
        else {
            if (end_of_path[1]){
                utils::strlcpy(basename, end_of_path + 1, basename_len);
                // default extension : leave whatever is in extension output buffer
            }
            else {
                // default name : leave whatever is in name output buffer
                // default extension : leave whatever is in extension output buffer
            }
        }
    }
    else {
        // default path : leave whatever is in path output buffer
        const char * start_of_extension = strrchr(fullpath, '.');
        if (start_of_extension){
            utils::strlcpy(extension, start_of_extension, extension_len);
            if (start_of_extension > fullpath){
                if (static_cast<size_t>(start_of_extension - fullpath) <= basename_len) {
                    memcpy(basename, fullpath, start_of_extension - fullpath);
                    basename[start_of_extension - fullpath] = 0;
                }
                else {
                    LOG(LOG_ERR, "canonical_path : basename too long for the buffer");
                    return false;
                }
            }
            // else no basename : leave output buffer for name untouched
        }
        else {
            if (fullpath[0]){
                utils::strlcpy(basename, fullpath, basename_len);
                // default extension : leave whatever is in extension output buffer
            }
            else {
                // default name : leave whatever is in name output buffer
                // default extension : leave whatever is in extension output buffer
            }
        }
    }
    return true;
}


static int internal_make_directory(const char *directory, mode_t mode)
{
    struct stat st;
    int status = 0;

    if (directory[0] != 0 && !dirname_is_dot(directory)) {
        if (stat(directory, &st) != 0) {
            /* Directory does not exist. */
            if ((mkdir(directory, mode) != 0) && (errno != EEXIST)) {
                status = -1;
                LOG(LOG_ERR, "failed to create directory %s: %s [%d]", directory, strerror(errno), errno);
            }
        }
        else if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            LOG(LOG_ERR, "expecting directory name, got filename, for %s", directory);
            status = -1;
        }
    }
    return status;
}


int recursive_create_directory(const char * directory, mode_t mode)
{
    if (!directory) {
        LOG(LOG_ERR, "Call to recursive create directory without directory path (null)");
        return -1;
    }

    int status = 0;
    std::string copy_directory = directory;
    char * pSearch;

    for (char * pTemp = copy_directory.data()
        ; (status == 0) && ((pSearch = strchr(pTemp, '/')) != nullptr)
        ; pTemp = pSearch + 1) {
        if (pSearch == pTemp) {
            continue;
        }

        pSearch[0] = 0;
        status = internal_make_directory(copy_directory.data(), mode);
        *pSearch = '/';
    }

    // creation of last directory in chain or nothing if path ending with slash
    if (status == 0 && *directory != 0) {
        status = internal_make_directory(directory, mode);
    }

    return status;
}

bool dirname_is_dot(not_null_ptr<char const> path) noexcept
{
    return (path[0] == '.' && path[1] == '\0')
        || (path[0] == '.' && path[1] == '.' && path[2] == '\0')
        ;
}

int recursive_delete_directory(const char * directory_path_char_ptr)
{
    // TODO: use string for recursive_delete_directory() parameter
    DIR * dir = opendir(directory_path_char_ptr);

    int return_value = 0;

    if (dir) {
        std::string entry_path = str_concat(directory_path_char_ptr, '/');
        const auto entry_path_len = entry_path.size();

        struct dirent * ent;

        while (!return_value && (ent = readdir(dir)))
        {
            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (dirname_is_dot(ent->d_name)) {
                continue;
            }

            entry_path.erase(entry_path_len);
            str_append(entry_path, ent->d_name);
            struct stat statbuf;

            if (0 == stat(entry_path.c_str(), &statbuf)) {
                if (S_ISDIR(statbuf.st_mode)) {
                    return_value = recursive_delete_directory(entry_path.c_str());
                }
                else {
                    return_value = unlink(entry_path.c_str());
                }
            }
        }

        closedir(dir);
    }

    if (!return_value) {
        return_value = rmdir(directory_path_char_ptr);
    }

    return return_value;
}

FileContentsError append_file_contents(const char * filename, std::string& buffer, off_t max_size)
{
    if (unique_fd ufd{open(filename, O_RDONLY)}) {
        struct stat statbuf;
        if (-1 == fstat(ufd.fd(), &statbuf)) {
            return FileContentsError::Stat;
        }

        ssize_t remaining = std::min(statbuf.st_size, max_size);
        buffer.resize(buffer.size() + remaining);
        auto* p = buffer.data() + buffer.size() - remaining;
        ssize_t r;
        while (0 < (r = read(ufd.fd(), p, remaining))) {
            remaining -= r;
            p += r;
        }

        if (remaining || r < 0) {
            return FileContentsError::Read;
        }

        return FileContentsError::None;
    }

    return FileContentsError::Open;
}

FileContentsError append_file_contents(std::string const& filename, std::string& buffer, off_t max_size)
{
    return append_file_contents(filename.c_str(), buffer, max_size);
}
