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

#include "bin.h"
#include "helpers.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

static void go(FILE * const f, const u32 size) {

}

int main(int argc, char **argv) {

	if (argc < 2)
		die("Usage: %s file.bin\n", argv[0]);

	FILE *f = fopen(argv[1], "r");
	if (!f) die("Failed to open file\n");

	struct stat st;
	fstat(fileno(f), &st);

	char magic[MAGICLEN];
	sread(magic, MAGICLEN, f);

	if (memcmp(magic, MAGIC, MAGICLEN))
		die("This is not a bostats binary file.\n");

	u16 buffers;
	sread(&buffers, 2, f);

	FILE *p = NULL;
	if (isatty(STDOUT_FILENO))
		p = popen("less", "w");
	if (p)
		dup2(STDOUT_FILENO, fileno(p));

	printf("%u buffers found\n", buffers);

	go(f, st.st_size);

	if (p)
		pclose(p);
	fclose(f);
	return 0;
}
