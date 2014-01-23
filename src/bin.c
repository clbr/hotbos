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

#include "lrtypes.h"
#include "bin.h"
#include "helpers.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

void readentry(entry * const e, void * const in, const u8 charbufs, u32 * const lasttime) {

	u32 tmp = 0;
	u8 buf[8];
	sgzread(buf, charbufs + 1, in);

	memset(e, 0, sizeof(entry));

	tmp = buf[0];
	e->id = (tmp >> 5) & 7;
	const u32 reltime = (tmp & 0x1f);
	e->time = reltime + *lasttime;
	*lasttime += reltime;

	((u8 *) &e->buffer)[0] = buf[1];
	if (charbufs == 2) {
		((u8 *) &e->buffer)[1] = buf[2];
	} else if (charbufs == 3) {
		((u8 *) &e->buffer)[2] = buf[3];
	}

	switch(e->id) {
		case ID_CREATE:
			sgzread(&tmp, 4, in);
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

void *gzbinopen(const char in[], u32 * const size) {

	void * const f = gzopen(in, "rb");
	if (!f) die("Failed to open file\n");

	struct stat st;
	if (stat(in, &st))
		die("Failed in stat\n");
	*size = st.st_size;

	char magic[MAGICLEN];
	sgzread(magic, MAGICLEN, f);
	if (memcmp(magic, MAGIC, MAGICLEN))
		die("This is not a bostats binary file.\n");

	return f;
}
