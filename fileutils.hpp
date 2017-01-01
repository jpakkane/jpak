/*
 * Copyright (C) 2016-2017 Jussi Pakkanen.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 3, or (at your option) any later version,
 * of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include<vector>
#include<string>
#include<cstdint>

struct fileinfo {
    uint64_t uncompressed_size;
    uint64_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t atime;
    uint32_t mtime;
    std::string fname;
    // FIXME missing checksum.
};

std::vector<fileinfo> expand_files(const std::vector<std::string> &originals);

bool is_dir(const fileinfo &f);
bool is_file(const fileinfo &f);
