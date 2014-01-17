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
#include <limits.h>

/* ANSI Colors */
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"

#define ANSI_CYAN          "\033[36m"
#define ANSI_BOLD_CYAN     ANSI_BOLD ANSI_CYAN
#define ANSI_MAGENTA       "\033[35m"
#define ANSI_BOLD_MAGENTA  ANSI_BOLD ANSI_MAGENTA
#define ANSI_RED           "\033[31m"
#define ANSI_BOLD_RED      ANSI_BOLD ANSI_RED
#define ANSI_YELLOW        "\033[33m"
#define ANSI_BOLD_YELLOW   ANSI_BOLD ANSI_YELLOW
#define ANSI_BLUE          "\033[34m"
#define ANSI_BOLD_BLUE     ANSI_BOLD ANSI_BLUE
#define ANSI_GREEN         "\033[32m"
#define ANSI_BOLD_GREEN    ANSI_BOLD ANSI_GREEN
#define ANSI_WHITE         "\033[37m"
#define ANSI_BOLD_WHITE    ANSI_BOLD ANSI_WHITE

#define NUMBER_HL ANSI_YELLOW

static void go(FILE * const f, const u32 size, const u8 charbufs) {

	long pos;
	entry e;

	for (pos = ftell(f); pos < size; pos = ftell(f)) {
		readentry(&e, f, charbufs);

		switch (e.id) {
			case ID_CREATE:
				printf("%screate", ANSI_CYAN);
			break;
			case ID_READ:
				printf("%sread", ANSI_MAGENTA);
			break;
			case ID_WRITE:
				printf("%swrite", ANSI_YELLOW);
			break;
			case ID_DESTROY:
				printf("%sdestroy", ANSI_RED);
			break;
			case ID_CPUOP:
				printf("%scpu op", ANSI_GREEN);
			break;
		}

		if (e.id == ID_CREATE) {
			printf("%s buffer %s%u%s at %s%u%s ms (%s%u%s bytes%s)\n",
				ANSI_RESET,
				NUMBER_HL, e.buffer, ANSI_RESET,
				NUMBER_HL, e.time, ANSI_RESET,
				NUMBER_HL, e.size, ANSI_RESET,
				e.high_prio ? ", " ANSI_YELLOW "high priority" ANSI_RESET : "");
		} else {
			printf("%s buffer %s%u%s at %s%u%s ms\n", ANSI_RESET,
				NUMBER_HL, e.buffer, ANSI_RESET, NUMBER_HL,
				e.time, ANSI_RESET);
		}
	}

	fflush(stdout);
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
	if (isatty(STDOUT_FILENO)) {
		p = popen("less -R", "w");
		if (!p)
			p = popen("less", "w");

		if (p)
			dup2(fileno(p), STDOUT_FILENO);
	}

	printf("%u buffers found\n", buffers);
	fflush(stdout);

	go(f, st.st_size, buffers < UCHAR_MAX);

	fclose(stdout);
	if (p)
		pclose(p);
	fclose(f);
	return 0;
}
