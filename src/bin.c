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

#define _GNU_SOURCE

#include "lrtypes.h"
#include "bin.h"
#include "helpers.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

void *gzbinopen(const char in[]) {

	void * const f = gzopen(in, "rb");
	if (!f) die("Failed to open file %s\n", in);

	char magic[MAGICLEN];
	sgzread(magic, MAGICLEN, f);
	if (memcmp(magic, MAGIC, MAGICLEN))
		die("This is not a bostats binary file.\n");

	return f;
}

void bincache(void * const f, char **ptr, u32 *size) {

	enum {
		bufsize = 1024 * 1024
	};

	size_t s;
	char buf[bufsize];
	FILE * const tmp = open_memstream(ptr, &s);
	while (1) {
		int ret = gzread(f, buf, bufsize);
		if (!ret) break;

		swrite(buf, ret, tmp);
	}
	fclose(tmp);

	*size = s;
}
