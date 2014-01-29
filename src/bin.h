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
#include "helpers.h"
#include <stdio.h>
#include <zlib.h>
#include <limits.h>
#include <string.h>

/*

   All data is little-endian. No BE support.

   One entry takes two to four bytes, as follows:

	struct {
		u8 time: 5;
		u8 id: 3;
		u8/u16/u24 buffer;
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
	u32 buffer;
	u8 high_prio;
	u8 id;
} entry;

#define MAGIC "bostats1"
#define MAGICLEN sizeof(MAGIC) - 1

void *gzbinopen(const char in[]);
void bincache(void *, char **, u32 *);

static inline u8 getcharbuf(const u32 bufs) {

	if (bufs < UCHAR_MAX)
		return 1;
	if (bufs < USHRT_MAX)
		return 2;

	return 0;
}

static inline void readentry(entry * const e, const char * const in, const u8 charbufs,
		u32 * const lasttime) {

	u32 tmp = 0;
	memset(e, 0, sizeof(entry));

	tmp = in[0];
	e->id = (tmp >> 5) & 7;
	const u32 reltime = (tmp & 0x1f);
	e->time = reltime + *lasttime;
	*lasttime += reltime;

	((u8 *) &e->buffer)[0] = in[1];
	if (charbufs == 2) {
		((u8 *) &e->buffer)[1] = in[2];
	} else if (charbufs == 3) {
		((u8 *) &e->buffer)[1] = in[2];
		((u8 *) &e->buffer)[2] = in[3];
	}

	switch(e->id) {
		case ID_CREATE:
			memcpy(&tmp, in + charbufs + 1, 4);
			e->high_prio = tmp >> 31;
			e->size = tmp & 0x7fffffff;
		break;
		case ID_READ:
		case ID_WRITE:
		case ID_DESTROY:
		case ID_CPUOP:
		break;
		default:
			die("Unknown entry id %u\n", e->id);
	}
}

#endif
