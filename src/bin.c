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

static u32 lasttime = 0;

void readentry(entry * const e, void * const in, const u8 charbufs) {

	u32 tmp = 0;

	memset(e, 0, sizeof(entry));

	sgzread(&tmp, 1, in);
	e->id = (tmp >> 5) & 7;
	const u32 reltime = (tmp & 0x1f);
	e->time = reltime + lasttime;
	lasttime += reltime;

	if (charbufs == 1)
		sgzread(&e->buffer, 1, in);
	else if (charbufs == 2)
		sgzread(&e->buffer, 2, in);
	else
		sgzread(&e->buffer, 3, in);

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

void resetreading() {
	lasttime = 0;
}
