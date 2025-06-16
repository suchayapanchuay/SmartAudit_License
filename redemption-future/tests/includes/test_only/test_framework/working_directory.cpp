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
#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/impl/test_paths.hpp"

#include "working_directory.hpp"
#include "utils/fileutils.hpp"
#include "utils/pp.hpp"
#include "utils/strutils.hpp"
#include "utils/sugar/scope_exit.hpp"
#include "cxx/compiler_version.hpp"
#include "cxx/cxx.hpp"

#include <charconv>
#include <vector>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <exception>
#include <string_view>

#include <cerrno>
#include <cassert>
#include <cstring>
#include <cstdio>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>


namespace
{
    std::string suffix_by_test(std::string_view name)
    {
        return ut_impl::compute_test_path(name, '/');
    }

#define WD_ERROR_S(ostream_expr) RED_ERROR("WorkingDirectory: " ostream_expr)
#define WD_ERROR(ostream_expr) RED_ERROR("WorkingDirectory: " << ostream_expr)
}

std::ostream& operator<<(std::ostream& out, WorkingFileBase const& wf)
{
    return out << wf.string();
}

WorkingFile::WorkingFile(std::string_view name)
: WorkingFileBase(suffix_by_test(""))
, start_error_count(RED_ERROR_COUNT())
{
    recursive_delete_directory(this->c_str());
    if (-1 == mkdir(this->c_str(), 0755) && errno != EEXIST) {
        WD_ERROR(strerror(errno) << ": " << this->string());
    }

    str_append(this->filename_, name);
}

WorkingFile::~WorkingFile()
{
    if (!this->is_removed && ! std::uncaught_exceptions()) {
        if (this->start_error_count == RED_ERROR_COUNT()) {
            RED_TEST_FUNC(unlink, (this->filename_.c_str()) == 0);
            this->filename_.resize(this->filename_.find_last_of('/'));
            rmdir(this->filename_.c_str());
        }
        else {
            RED_TEST_FUNC(file_exist, (this->filename_.c_str()));
        }
    }
}


WorkingDirectory::SubDirectory::SubDirectory(
    WorkingDirectory& wd, std::string fullpath, std::size_t dirname_pos)
: wd_(wd)
, fullpath(std::move(fullpath))
, dirname_pos(dirname_pos)
{}

std::string_view WorkingDirectory::SubDirectory::subdirname() const noexcept
{
    auto* s = this->fullpath.c_str();
    return {s + this->dirname_pos, this->fullpath.size() - this->dirname_pos};
}

WorkingFileBase WorkingDirectory::SubDirectory::add_file(std::string_view file)
{
    return this->wd_.add_file(str_concat(this->subdirname(), file));
}

WorkingDirectory::SubDirectory& WorkingDirectory::SubDirectory::add_files(
    std::initializer_list<std::string_view> files)
{
    for (auto sv : files) {
        (void)this->wd_.add_file_(str_concat(this->subdirname(), sv));
    }
    return *this;
}

WorkingDirectory::SubDirectory& WorkingDirectory::SubDirectory::remove_files(
    std::initializer_list<std::string_view> files)
{
    for (auto sv : files) {
        (void)this->wd_.remove_files({str_concat(this->subdirname(), sv)});
    }
    return *this;
}

std::string WorkingDirectory::SubDirectory::path_of(std::string_view path) const
{
    return this->wd_.path_of(str_concat(this->dirname(), path));
}

WorkingDirectory::SubDirectory
WorkingDirectory::SubDirectory::create_subdirectory(std::string_view dirname)
{
    return this->wd_.create_subdirectory(str_concat(this->subdirname(), dirname));
}


WorkingDirectory::Path::Path() noexcept = default;

WorkingDirectory::Path::Path(std::string name, int counter_id) noexcept
: name(std::move(name))
, type(this->name.back() == '/' ? Type::Directory : Type::File)
, counter_id(counter_id)
{}

bool WorkingDirectory::Path::operator == (Path const& other) const
{
    return this->name == other.name;
}


std::size_t WorkingDirectory::HashPath::operator()(Path const& path) const
{
    return std::hash<std::string>()(path.name);
}


WorkingDirectory::WorkingDirectory(std::string_view name)
: dirname_(suffix_by_test(name))
, start_error_count_(RED_ERROR_COUNT())
{
    recursive_delete_directory(this->dirname_);
    if (-1 == mkdir(this->dirname_, 0755) && errno != EEXIST) {
        WD_ERROR(strerror(errno) << ": " << this->dirname_);
    }
}

WorkingDirectory::SubDirectory WorkingDirectory::create_subdirectory(std::string_view dirname)
{
    auto pos = dirname.find_first_not_of('/');
    if (pos) {
        dirname = dirname.substr(pos+1);
    }

    if (dirname.empty()) {
        WD_ERROR_S("empty dirname");
    }

    if (dirname.back() == '/') {
        dirname = dirname.substr(0, dirname.size()-1);
    }

    auto path = this->add_file(str_concat(dirname, '/'));
    recursive_create_directory(path.c_str(), 0755);
    return SubDirectory(*this, std::move(path), this->dirname_.size());
}

std::string const& WorkingDirectory::add_file_(std::string file)
{
    auto [it, b] = this->paths_.emplace(std::move(file), this->counter_id_);
    if (!b) {
        this->has_error_ = true;
        WD_ERROR(it->name << " already exists");
    }
    this->is_checked_ = false;
    return it->name;
}

void WorkingDirectory::remove_file_(std::string file)
{
    // use is_transparent and std::string_view with C++23
    Path path(std::move(file), 0);
    if (!this->paths_.erase(path)) {
        this->has_error_ = true;
        WD_ERROR_S("unknown file '" << path.name << '\'');
    }
}

WorkingFileBase WorkingDirectory::add_file(std::string file)
{
    return WorkingFileBase(this->path_of(this->add_file_(std::move(file))));
}

WorkingDirectory& WorkingDirectory::add_files(std::initializer_list<std::string_view> files)
{
    for (auto const& sv : files) {
        this->add_file_(std::string(sv));
    }
    return *this;
}

void WorkingDirectory::remove_file(std::string file)
{
    this->remove_file_(std::move(file));
    this->is_checked_ = false;
}

void WorkingDirectory::remove_file(WorkingFileBase const& file)
{
    std::string_view path = file.string();
    if (utils::starts_with(path, this->dirname_.string())) {
        path.remove_prefix(this->dirname_.size());
    }
    this->remove_file_(std::string(path));
    this->is_checked_ = false;
}

WorkingDirectory& WorkingDirectory::remove_files(std::initializer_list<std::string_view> files)
{
    for (auto const& sv : files) {
        this->remove_file_(std::string(sv));
    }
    this->is_checked_ = false;
    return *this;
}

std::string WorkingDirectory::path_of(std::string_view path) const
{
    return str_concat(this->dirname_, path);
}

std::string WorkingDirectory::unmached_files()
{
    this->is_checked_ = true;
    ++this->counter_id_;

    Path path(this->dirname_, 0);
    Path filename;
    std::string err;
    std::vector<std::string> unknwon_list;
    std::vector<std::string> not_found_list;
    std::vector<std::string> unmatching_file_type_list;

    auto get_relative_path = [&](std::string_view const& s){
        return std::string(s.substr(this->dirname_.size()));
    };

    auto unmached_files_impl = [&](auto recursive) -> bool
    {
        DIR * dir = opendir(path.name.c_str());
        if (!dir) {
            str_append(err, "opendir ", path.name, " error: ", strerror(errno), '\n');
            return false;
        }
        SCOPE_EXIT(closedir(dir));

        auto const original_path_len = path.name.size();
        auto const original_filename_len = filename.name.size();

        bool has_entry = false;

        struct stat statbuf {};
        while (struct dirent* ent = readdir(dir)) {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
                continue;
            }

            has_entry = true;

            path.name += ent->d_name;
            filename.name += ent->d_name;

            auto const type = (!stat(path.name.c_str(), &statbuf) && S_ISDIR(statbuf.st_mode))
              ? Type::Directory : Type::File;

            if (type == Type::Directory) {
                path.name += '/';
                filename.name += '/';
            }

            auto it = this->paths_.find(filename);
            if (it == this->paths_.end()) {
                if (type != Type::Directory || !recursive(recursive)) {
                    unknwon_list.push_back(get_relative_path(path.name));
                }
            }
            else {
                it->counter_id = this->counter_id_;
                if (it->type != type) {
                    unmatching_file_type_list.push_back(get_relative_path(path.name));
                }
                else if (type == Type::Directory){
                    recursive(recursive);
                }
            }

            path.name.resize(original_path_len);
            filename.name.resize(original_filename_len);
        }

        return has_entry;
    };

    unmached_files_impl(unmached_files_impl);

    for (auto const& p : this->paths_) {
        if (p.counter_id != this->counter_id_) {
            not_found_list.push_back(p.name);
        }
    }

    bool not_empty_lists
      = !unknwon_list.empty()
     || !not_found_list.empty()
     || !unmatching_file_type_list.empty();

    this->has_error_ = this->has_error_ || !err.empty() || not_empty_lists;

    if (not_empty_lists) {
        struct P {
            std::string_view header;
            std::vector<std::string>& file_list;
        };
        using namespace std::string_view_literals;

        for (auto prefix : {""sv, std::string_view(this->dirname_)}) {
            if (prefix.empty()) {
                str_append(err, "\n(relative path)"sv);
            }
            else {
                str_append(err, "\n\n(full path)\n", prefix);
            }

            for (P const& p : {
                P{"\n-> unknown ", unknwon_list},
                P{"\n-> not found ", not_found_list},
                P{"\n-> unmatching file type ", unmatching_file_type_list},
            }) {
                if (!p.file_list.empty()) {
                    char buf[16];
                    auto beg = std::begin(buf);
                    auto end = std::to_chars(beg, std::end(buf), p.file_list.size()).ptr;
                    str_append(err, p.header, '(', std::string_view(beg, end-beg), "):");
                    for (auto& file : p.file_list) {
                        str_append(err, "\n - ", prefix, file);
                    }
                }
            }
        }
        err += '\n';
    }

    return err;
}

WorkingDirectory::~WorkingDirectory() noexcept(false)
{
    if (!this->is_checked_ && this->start_error_count_ == RED_ERROR_COUNT()) {
        WD_ERROR_S("unchecked entries");
    }

    if (!this->has_error_
     && !std::uncaught_exceptions()
     && this->start_error_count_ == RED_ERROR_COUNT()
    ) {
        recursive_delete_directory(this->dirname_.c_str());
    }
}
