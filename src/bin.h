/*
Copyright (C) 2013 Lauri Kasanen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BIN_H
#define BIN_H

#include "lrtypes.h"
#include <stdio.h>
#include <zlib.h>

/* One entry takes four to five bytes, as follows:

	struct {
		u8 id: 3;
		u32 time: 21;
		u8/u16 buffer;
	}

   Create entries are followed by four bytes:

	struct {
		u8 high_prio: 1;
		u32 size: 31;
	}
*/

enum id_t {
	ID_CREATE = 0,
	ID_READ = 1,
	ID_WRITE = 2,
	ID_DESTROY = 3,
	ID_CPUOP = 4
};

typedef struct {
	u32 time;
	u32 size;
	enum id_t id;
	u16 buffer;
	u8 high_prio;
} entry;

#define MAGIC "bostats1"
#define MAGICLEN sizeof(MAGIC) - 1

void readentry(entry * const e, void * const in, const u8 charbufs);

#endif
