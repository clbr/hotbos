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
#include <limits.h>
#include <string.h>

enum {
	bufsize = 256,
	ptrsize = 11
};

static u8 charbufs = 0;
static u16 firstprio = USHRT_MAX;

static u16 buffers(FILE * const in) {

	char buf[bufsize];
	u32 total = 0;

	while (fgets(buf, bufsize, in)) {
		if (strstr(buf, "created")) {
			total++;
		}
	}

	rewind(in);

	if (total > USHRT_MAX) die("Too many buffers (%u)\n", total);
	if (total < UCHAR_MAX) charbufs = 1;

	return total;
}

static u8 hashptr(const char ptr[]) {

	// Get the first and second-to-last char, convert, combine
	const u8 first = hex2u8(ptr[0]);

	u8 i;
	for (i = 2; ptr[i + 2]; i++);

	const u8 penultimate = hex2u8(ptr[i]);

	return first | (penultimate << 4);
}

static u16 findbuf(const char ptr[], const u32 bufcount, char (*ptr2id)[ptrsize],
			const u8 hashes[]) {

	const u8 myhash = hashptr(ptr);

	u16 i;
	for (i = 0; i < bufcount; i++) {
		if (myhash != hashes[i])
			continue;

		if (!strcmp(ptr2id[i], ptr))
			return i;
	}

	printf("Buffer not found, assuming from_handle creation, missing in pre-jan-17 traces\n");
	return firstprio;

	//die("Asked for a non-existent buffer %s\n", ptr);
}

static void output(const entry * const e, FILE * const out) {

	u32 tmp;
	static u32 lasttime = 0;

	const u32 reltime = e->time - lasttime;

	switch(e->id) {
		case ID_CREATE:
		case ID_READ:
		case ID_WRITE:
		case ID_DESTROY:
		case ID_CPUOP:
			tmp = e->id << 5 | (reltime & 0x1f);
			swrite(&tmp, 1, out);

			if (charbufs)
				swrite(&e->buffer, 1, out);
			else
				swrite(&e->buffer, 2, out);
		break;
		default:
			die("Unknown entry id %u\n", e->id);
	}

	if (e->id == ID_CREATE) {
		tmp = e->high_prio << 31 | e->size;
		swrite(&tmp, 4, out);
	}

	lasttime = e->time;
}

static void nukenewline(char buf[]) {

	char *ptr = strchr(buf, '\n');
	if (ptr)
		*ptr = '\0';
}

static void handle(FILE * const in, FILE * const out, const u16 bufcount) {

	printf("Total %u buffers created in the trace.\n", bufcount);

	swrite(MAGIC, MAGICLEN, out);
	swrite(&bufcount, 2, out);

	char buf[bufsize];
	entry e;
	u32 starttime = 0;
	u16 curbuf = 0;

	char ptr2id[bufcount][ptrsize];
	u8 hashes[bufcount];

/* started @4207333
   0x93cc630 created, size 65536, prio 0, @4207333
   0x93cc630 cpu mapped @4207333
   0x93cc630 read @4207338
   0x93cd290 write @4207338
   0x93cd320 destroyed @4207338
*/

	#define malformed die("Malformed line: %s (line %u)\n", buf, __LINE__)

	while (fgets(buf, bufsize, in)) {
		nukenewline(buf);
		memset(&e, 0, sizeof(entry));

		const char * const stamp = strchr(buf, '@');
		if (!stamp) die("No time stamp on line: %s\n", buf);

		u32 now;
		if (sscanf(stamp + 1, "%u", &now) != 1) malformed;
		e.time = now - starttime;

		char ptr[ptrsize];
		if (buf[0] == '0') {
			char *space = strchr(buf, ' ');
			if (!space) malformed;
			u32 len = space - buf - 2;
			memcpy(ptr, buf + 2, len);
			ptr[len] = '\0';
		}

		u8 getbuf = 1;

		// Handling
		if (strstr(buf, "started")) {
			starttime = now;
			continue;
		} else if (strstr(buf, "created")) {
			memcpy(ptr2id[curbuf], ptr, ptrsize);
			hashes[curbuf] = hashptr(ptr);
			e.buffer = curbuf;
			e.id = ID_CREATE;

			char *start = strchr(buf, ',');
			if (!start) malformed;

			if (sscanf(start, ", size %u, prio %hhu,", &e.size, &e.high_prio) != 2)
				malformed;

			if (e.high_prio && firstprio == USHRT_MAX)
				firstprio = curbuf;

			curbuf++;
			getbuf = 0;
		} else if (strstr(buf, "cpu mapped")) {
			e.id = ID_CPUOP;
		} else if (strstr(buf, "read")) {
			e.id = ID_READ;
		} else if (strstr(buf, "write")) {
			e.id = ID_WRITE;
		} else if (strstr(buf, "destroyed")) {
			e.id = ID_DESTROY;
		} else {
			die("Unrecognized line: %s\n", buf);
		}

		if (getbuf)
			e.buffer = findbuf(ptr, curbuf, ptr2id, hashes);

		output(&e, out);
	}
}

int main(int argc, char **argv) {

	if (argc < 2) {
		die("Usage: %s src [dest]\n", argv[0]);
	}

	const char * const src = argv[1];
	char *dest;
	if (argc > 2) {
		dest = argv[2];
	} else {
		u32 len = strlen(src) + 5;
		dest = xcalloc(len);
		memcpy(dest, src, len - 5);
		strcat(dest, ".bin");
	}

	FILE *in = fopen(src, "r");
	if (!in)
		die("Failed to open input file\n");

	FILE *out = fopen(dest, "w");
	if (!out)
		die("Failed to open output file\n");

	handle(in, out, buffers(in));

	fclose(in);
	fclose(out);

	if (dest != argv[2]) free(dest);

	return 0;
}
