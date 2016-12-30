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

#include<jpacker.hpp>
#include<file.hpp>

#include<cstdio>
#include<cassert>

void jpack(const char *ofname, const std::vector<fileinfo> &entries) {
    File ofile(ofname, "wb");
    ofile.write("JPAK0", 4);
    std::vector<uint64_t> entry_offsets;
    entry_offsets.reserve(entries.size());
    for(const auto &e : entries) {
        entry_offsets.push_back(ofile.tell());
        if(is_dir(e)) {
            continue;
        }
        File ifile(e.fname, "r");
        ofile.append(ifile);
        // FIXME, store compressed size.
    }
    assert(entry_offsets.size() == entries.size());
    uint64_t index_offset = ofile.tell();
    // Now dump metadata one column at a time for maximal compression.
    for(const auto &e : entries) {
        ofile.write64le(e.uncompressed_size);
    }
    for(const auto &e : entries) {
        ofile.write64le(e.compressed_size);
    }
    for(const auto &e : entries) {
        ofile.write64le(e.mode);
    }
    for(const auto &e : entries) {
        ofile.write32le(e.uid);
    }
    for(const auto &e : entries) {
        ofile.write32le(e.gid);
    }
    for(const auto &e : entries) {
        ofile.write32le(e.atime);
    }
    for(const auto &e : entries) {
        ofile.write32le(e.mtime);
    }
    for(const auto &e : entries) {
        ofile.write16le(e.fname.size());
    }
    for(const auto &o : entry_offsets) {
        ofile.write64le(o);
    }
    // Filenames have variable length so they must be last.
    for(const auto &e : entries) {
        ofile.write(e.fname);
    }
    // Now done. Write suffix.
    ofile.write32le(12345678);
    ofile.write64le(entries.size());
    ofile.write64le(index_offset);
    // FIXME when compressing index, write also end of it.
}

