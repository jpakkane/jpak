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

#include"utils.hpp"

#if _WIN32
#include<winsock2.h>
#include<windows.h>
#endif

#include<cerrno>
#include<cassert>
#include<cstring>

#include<stdexcept>
#include<string>

#ifndef _WIN32
using std::min;
#endif

void throw_system(const char *msg) {
    std::string error(msg);
    assert(errno != 0);
    if(error.back() != ' ') {
        error += ' ';
    }
    error += strerror(errno);
    throw std::runtime_error(error);
}
