/*
 * Copyright (C) 2016 Jussi Pakkanen.
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

#include<fileutils.hpp>
#include<utils.hpp>
#include<numeric>

#ifdef _WIN32
#include<WinSock2.h>
#include<windows.h>
#include<direct.h>
#else
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#endif

#include<memory>
#include<algorithm>

namespace {

std::vector<fileinfo> expand_entry(const std::string &fname);

fileinfo get_unix_stats(const std::string &fname) {
    struct stat buf;
    fileinfo sd;
#ifdef _WIN32
    if (stat(fname.c_str(), &buf) != 0) {
#else
    if(lstat(fname.c_str(), &buf) != 0) {
#endif
        throw_system("Could not get entry stats: ");
    }
    sd.fname = fname;
    sd.uid = buf.st_uid;
    sd.gid = buf.st_gid;
#if defined(__APPLE__)
    sd.atime = buf.st_atimespec.tv_sec;
    sd.mtime = buf.st_mtimespec.tv_sec;
#elif defined(_WIN32)
    sd.atime = buf.st_atime;
    sd.mtime = buf.st_mtime;
#else
    sd.atime = buf.st_atim.tv_sec;
    sd.mtime = buf.st_mtim.tv_sec;
#endif
    sd.mode = buf.st_mode;
    sd.uncompressed_size = buf.st_size;
//    sd.device_id = buf.st_rdev;
    return sd;
}

#ifdef _WIN32
std::vector<std::string> handle_dir_platform(const std::string &dirname) {
    std::string glob = dirname + "\\*.*";
    std::vector<std::string> entries;
    HANDLE hFind;
    WIN32_FIND_DATA data;

    hFind = FindFirstFile(glob.c_str(), &data);
    if (hFind == INVALID_HANDLE_VALUE) {
        throw_system("Could not get directory contents: ");
    }
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            entries.push_back(data.cFileName);
        } while(FindNextFile(hFind, &data));
        FindClose(hFind);
    }
    return entries;
}

#else

std::vector<std::string> handle_dir_platform(const std::string &dirname) {
    std::vector<std::string> entries;
    std::unique_ptr<DIR, int(*)(DIR*)> dirholder(opendir(dirname.c_str()), closedir);
    auto dir = dirholder.get();
    if(!dir) {
        printf("Could not access directory: %s\n", dirname.c_str());
        return entries;
    }
    std::array<char, sizeof(dirent) + NAME_MAX + 1> buf;
    struct dirent *cur = reinterpret_cast<struct dirent*>(buf.data());
    struct dirent *de;
    std::string basename;
    while (readdir_r(dir, cur, &de) == 0 && de) {
        basename = cur->d_name;
        if (basename == "." || basename == "..") {
            continue;
        }
        entries.push_back(basename);
    }
    return entries;
}
#endif

std::vector<fileinfo> expand_dir(const std::string &dirname) {
        // Always set order to create reproducible files.
        std::vector<fileinfo> result;
        auto entries = handle_dir_platform(dirname);
        std::sort(entries.begin(), entries.end());
        std::string fullpath;
        for(const auto &base : entries) {
            fullpath = dirname + '/' + base;
            auto new_ones = expand_entry(fullpath);
            std::move(new_ones.begin(), new_ones.end(), std::back_inserter(result));
        }
        return result;
    }

std::vector<fileinfo> expand_entry(const std::string &fname) {
    auto fi = get_unix_stats(fname);
    std::vector<fileinfo> result{fi};
    if(is_dir(fi)) {
        auto new_ones = expand_dir(fname);
        std::move(new_ones.begin(), new_ones.end(), std::back_inserter(result));
        return result;
    }
    if(is_file(fi)) {
        return result;
    }
    return std::vector<fileinfo>{};
}

}

std::vector<fileinfo> expand_files(const std::vector<std::string> &originals) {
    return std::accumulate(originals.begin(), originals.end(), std::vector<fileinfo>{}, [](std::vector<fileinfo> res, const std::string &s) {
        auto n = expand_entry(s);
        std::move(n.begin(), n.end(), std::back_inserter(res));
        return res;
    });
}

bool is_dir(const fileinfo &f) {
    return S_ISDIR(f.mode);
}

bool is_file(const fileinfo &f) {
    return S_ISREG(f.mode);
}
