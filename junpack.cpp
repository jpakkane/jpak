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

#include<file.hpp>
#include<fileutils.hpp>
#include<mmapper.hpp>
#include<utils.hpp>

#include<fcntl.h>
#include<sys/stat.h>
#include<cstdio>

#include<lzma.h>

#include<memory>

namespace {

void lzma_to_file(const unsigned char *data_start,
                      uint64_t data_size,
                      FILE *ofile) {
    const int CHUNK=1024*1024;
    std::unique_ptr<unsigned char[]> out(new unsigned char [CHUNK]);
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_filter filter[2];
    unsigned int have;

    size_t offset = 2;
    uint16_t properties_size = le16toh(*reinterpret_cast<const uint16_t*>(data_start + offset));
    offset+=2;
    filter[0].id = LZMA_FILTER_LZMA1;
    filter[1].id = LZMA_VLI_UNKNOWN;
    lzma_ret ret = lzma_properties_decode(&filter[0], nullptr, data_start + offset, properties_size);
    offset += properties_size;
    if(ret != LZMA_OK) {
        throw std::runtime_error("Could not decode LZMA properties.");
    }
    ret = lzma_raw_decoder(&strm, &filter[0]);
    free(filter[0].options);
    if(ret != LZMA_OK) {
        throw std::runtime_error("Could not initialize LZMA decoder.");
    }
    std::unique_ptr<lzma_stream, void(*)(lzma_stream*)> lcloser(&strm, lzma_end);

    const unsigned char *current = data_start + offset;
    strm.avail_in = (size_t)(data_size - offset);
    strm.next_in = current;
    /* decompress until data ends */
    do {
        if (strm.total_in == data_size - offset)
            break;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out.get();
            ret = lzma_code(&strm, LZMA_RUN);
            if(ret != LZMA_OK && ret != LZMA_STREAM_END) {
                throw std::runtime_error("Decompression failed.");
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out.get(), 1, have, ofile) != have || ferror(ofile)) {
                throw_system("Could not write to file:");
            }
        } while (strm.avail_out == 0);
    } while (true);
}


uint64_t get_block_end(const std::vector<uint64_t> entry_offsets,
        uint64_t j,
        const uint64_t index_offset) {
    ++j;
    while(j < entry_offsets.size()) {
        if(entry_offsets[j] != NO_OFFSET) {
            return entry_offsets[j];
        }
        ++j;
    }
    return index_offset;
}

}

void unpack(const char *fname, std::string outdir) {
    File ifile(fname, "rb");
    std::vector<fileinfo> entries;
    std::vector<uint16_t> fname_sizes;
    std::vector<uint64_t> entry_offsets;
    ifile.seek(-28, SEEK_END);
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
    auto index_compressed_size = ifile.read64le();
    entries.reserve(num_entries);
    fname_sizes.reserve(num_entries);
    entry_offsets.reserve(num_entries);
    for(uint64_t j=0; j<num_entries; j++) {
        fileinfo f;
        entries.push_back(f);
    }
    printf("This file has %d entries.\n", (int)num_entries);
//    printf("Index size: %d\n", (int)index_compressed_size);

    auto mmap = ifile.mmap();
    File index(tmpfile());
    lzma_to_file((unsigned char*)mmap + index_offset, index_compressed_size, index.get());
    index.seek(0, SEEK_SET);
    for(auto &e : entries) {
        e.uncompressed_size = index.read64le();
    }
    for(auto &e : entries) {
        e.mode = index.read64le();
    }
    for(auto &e : entries) {
        e.uid = index.read32le();
    }
    for(auto &e : entries) {
        e.gid = index.read32le();
    }
    for(auto &e : entries) {
        e.atime = index.read32le();
    }
    for(auto &e : entries) {
        e.mtime = index.read32le();
    }
    for(uint64_t j=0; j<num_entries; j++) {
        fname_sizes.push_back(index.read16le());
    }
    for(uint64_t j=0; j<num_entries; j++) {
        entry_offsets.push_back(index.read64le());
    }
    // Filenames have variable length so they must be last.
    for(uint64_t j=0; j<num_entries; j++) {
        entries[j].fname = index.read(fname_sizes[j]);
    }

    File unpack_file(tmpfile());
    for(uint64_t j=0; j<num_entries; j++) {
        auto &e = entries[j];
        auto offset = entry_offsets[j];
        auto ofname = outdir + e.fname;
        printf("%s\n", e.fname.c_str());
        if(is_dir(e)) {
            mkdir(ofname.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            continue;
        }
        if(offset != NO_OFFSET) {
            // Wasteful, writes to temp file. Should be able to write directly to
            // outfile instead.
            auto block_end = get_block_end(entry_offsets, j, index_offset); // Assumes index immediately follows data.
            unpack_file.clear();
            lzma_to_file(mmap + offset, block_end - offset, unpack_file.get());
            unpack_file.flush();
            unpack_file.seek(0, SEEK_SET);
//            printf("Created temp file of size %d.\n", (int)unpack_file.size());
        } else {
//            printf("Continuing with existing unpackfile.\n");
        }
        File ofile(ofname, "wb");
        ofile.copy_from(unpack_file, e.uncompressed_size);
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
