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
#include<jpacker.hpp>
#include<cstdio>

int main(int argc, char **argv) {
    if(argc < 3) {
        printf("%s [jpack file] [files to package].\n", argv[0]);
        return 1;
    }
    std::vector<std::string> originals;
    for(int i=2; i<argc; i++) {
        originals.push_back(argv[i]);
    }
    auto entries = expand_files(originals);
    /*
    for(const auto &i : entries) {
        printf("%s\n", i.fname.c_str());
    }*/
    jpack(argv[1], entries);
    return 0;
}
