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

#include<file.hpp>
#include<fileutils.hpp>
#include<mmapper.hpp>
#include<fcntl.h>
#include<sys/stat.h>
#include<cstdio>

void unpack(const char *fname, std::string outdir) {
    File ifile(fname, "rb");
    std::vector<fileinfo> entries;
    std::vector<uint16_t> fname_sizes;
    std::vector<uint64_t> entry_offsets;
    ifile.seek(-20, SEEK_END);
    auto magic = ifile.read32le();
    if(magic != 12345678) {
        printf("Bad magic number, invalid archive.\n");
        return;
    }
    if(outdir.empty()) {
        printf("Extraction dir must not be empty.\n");
        return;
    }
    if(outdir.back() != '/') {
        outdir.push_back('/');
    }
    auto num_entries = ifile.read64le();
    auto index_offset = ifile.read64le();
    printf("This file has %d entries.\n", (int)num_entries);
    entries.reserve(num_entries);
    fname_sizes.reserve(num_entries);
    entry_offsets.reserve(num_entries);
    ifile.seek(index_offset, SEEK_SET);
    for(uint64_t j=0; j<num_entries; j++) {
        fileinfo f;
        entries.push_back(f);
    }
    for(auto &e : entries) {
        e.uncompressed_size = ifile.read64le();
    }
    for(auto &e : entries) {
        e.compressed_size = ifile.read64le();
    }
    for(auto &e : entries) {
        e.mode = ifile.read64le();
    }
    for(auto &e : entries) {
        e.uid = ifile.read32le();
    }
    for(auto &e : entries) {
        e.gid = ifile.read32le();
    }
    for(auto &e : entries) {
        e.atime = ifile.read32le();
    }
    for(auto &e : entries) {
        e.mtime = ifile.read32le();
    }
    for(uint64_t j=0; j<num_entries; j++) {
        fname_sizes.push_back(ifile.read16le());
    }
    for(uint64_t j=0; j<num_entries; j++) {
        entry_offsets.push_back(ifile.read64le());
    }
    // Filenames have variable length so they must be last.
    for(uint64_t j=0; j<num_entries; j++) {
        entries[j].fname = ifile.read(fname_sizes[j]);
    }
    /*
    for(const auto &e : entries) {
        printf("%s\n %d", e.fname.c_str());
    }
    */
    auto mmap = ifile.mmap();
    const unsigned char *start = mmap;
    for(uint64_t j=0; j<num_entries; j++) {
        auto &e = entries[j];
        auto offset = entry_offsets[j];
        auto ofname = outdir + e.fname;
        if(is_dir(e)) {
            mkdir(ofname.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        } else {
            File f(ofname, "wb");
            f.write(start + offset, e.uncompressed_size);
        }
        // FIXME restore metadata here.
    }
}

int main(int argc, char **argv) {
    if(argc != 3) {
        printf("%s <archive> <outdir>\n", argv[0]);
        return 1;
    }
    unpack(argv[1], argv[2]);
    return 0;
}
