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

#include<jpacker.hpp>
#include<file.hpp>
#include<mmapper.hpp>
#include<utils.hpp>

#include<lzma.h>

#include<memory>
#include<cstdio>
#include<cassert>

namespace {

File compress_lzma(MMapper buf) {
    const int CHUNK=1024*1024;
    FILE *tmpf = tmpfile();
    std::unique_ptr<unsigned char[]> out(new unsigned char [CHUNK]);
    uint32_t filter_size;
    if(!tmpf) {
        throw_system("Could not create temp file: ");
    }
    File f(tmpf);
    lzma_options_lzma opt_lzma;
    lzma_stream strm = LZMA_STREAM_INIT;
    if(lzma_lzma_preset(&opt_lzma, LZMA_PRESET_DEFAULT)) {
        throw std::runtime_error("Unsupported LZMA preset.");
    }
    lzma_filter filter[2];
    filter[0].id = LZMA_FILTER_LZMA1;
    filter[0].options = &opt_lzma;
    filter[1].id = LZMA_VLI_UNKNOWN;

    lzma_ret ret = lzma_raw_encoder(&strm, filter);
    if(ret != LZMA_OK) {
        throw std::runtime_error("Could not create LZMA encoder.");
    }
    if(lzma_properties_size(&filter_size, filter) != LZMA_OK) {
        throw std::runtime_error("Could not determine LZMA properties size.");
    } else {
        std::string x(filter_size, 'X');
        if(lzma_properties_encode(filter, (unsigned char*)x.data()) != LZMA_OK) {
            throw std::runtime_error("Could not encode filter properties.");
        }
        f.write8(9); // This is what Python's lzma lib does. Copy it without understanding.
        f.write8(4);
        f.write16le(filter_size);
        f.write(x);
    }
    std::unique_ptr<lzma_stream, void(*)(lzma_stream*)> lcloser(&strm, lzma_end);

    strm.avail_in = buf.size();
    strm.next_in = buf;
    /* compress until data ends */
    lzma_action action = LZMA_RUN;
    while(true) {
        if(strm.total_in >= buf.size()) {
            action = LZMA_FINISH;
        } else {
            strm.avail_out = CHUNK;
            strm.next_out = out.get();
        }
        ret = lzma_code(&strm, action);
        if(strm.avail_out == 0 || ret == LZMA_STREAM_END) {
            size_t write_size = CHUNK - strm.avail_out;
            if (fwrite(out.get(), 1, write_size, f) != write_size || ferror(f)) {
                throw_system("Could not write to file:");
            }
            strm.next_out = out.get();
            strm.avail_out = CHUNK;
        }

        if(ret != LZMA_OK) {
            if(ret == LZMA_STREAM_END) {
                break;
            }
            throw std::runtime_error("Compression failed.");
        }
    }

    f.flush();
    return f;
}

}


void jpack(const char *ofname, const std::vector<fileinfo> &entries) {
    File ofile(ofname, "wb");
    ofile.write("JPAK0", 4);
    std::vector<uint64_t> entry_offsets;
    entry_offsets.reserve(entries.size());
    // The proper way is to make lzma compressor a class that can be fed multiple files.
    // This copies data over, but meh.
    uint64_t stored_data = 0;
    File gather_file(tmpfile());
    // Bigger blocks improve compression but makes accessing single entries slower.
    const uint64_t block_size = 1024*1024;
    bool first_file_written = false;
    for(auto &e : entries) {
        uint64_t cur_offset = NO_OFFSET;
        if(is_dir(e)) {
            entry_offsets.push_back(NO_OFFSET);
            continue;
        }
        // Compress data if there is more of it than the specified clump size.
        if(stored_data >= block_size || !first_file_written) {
            if(first_file_written) {
                auto tmpfile = compress_lzma(gather_file.mmap());
                ofile.append(tmpfile);
                gather_file.clear();
                printf("Starting new tmpfile.\n");
                stored_data = 0;
            }
            cur_offset = ofile.tell();
            first_file_written = true;
        }
        File ifile(e.fname, "r");
        gather_file.append(ifile);
        stored_data += ifile.size();
        entry_offsets.push_back(cur_offset);
    }
    if(stored_data > 0) {
        auto tmpfile = compress_lzma(gather_file.mmap());
        ofile.append(tmpfile);
    }
    assert(entry_offsets.size() == entries.size());
    uint64_t index_offset = ofile.tell();
    File index(tmpfile());
    // Now dump metadata one column at a time for maximal compression.
    for(const auto &e : entries) {
        index.write64le(e.uncompressed_size);
    }
    for(const auto &e : entries) {
        index.write64le(e.mode);
    }
    for(const auto &e : entries) {
        index.write32le(e.uid);
    }
    for(const auto &e : entries) {
        index.write32le(e.gid);
    }
    for(const auto &e : entries) {
        index.write32le(e.atime);
    }
    for(const auto &e : entries) {
        index.write32le(e.mtime);
    }
    for(const auto &e : entries) {
        index.write16le(e.fname.size());
    }
    for(const auto &o : entry_offsets) {
        index.write64le(o);
    }
    // Filenames have variable length so they must be last.
    for(const auto &e : entries) {
        index.write(e.fname);
    }
    auto compressed_index = compress_lzma(index.mmap());
//    printf("Index uncompressed: %d\n", (int)index.size());
//    printf("Index compressed: %d\n", (int)compressed_index.tell());
    ofile.append(compressed_index);
    // Now done. Write suffix.
    ofile.write32le(12345678);
    ofile.write64le(entries.size());
    ofile.write64le(index_offset);
    ofile.write64le(compressed_index.size());
}

