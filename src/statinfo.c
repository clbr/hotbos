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

static void go(void * const f, const u32 size, const u8 charbufs) {

	entry e;
	const u8 direct = gzdirect(f);

	while (!gzeof(f)) {

		if (direct) {
			long pos = gztell(f);
			if (pos >= size) break;
		}

		readentry(&e, f, charbufs);

		switch (e.id) {
			case ID_CREATE:
				fputs_unlocked(ANSI_CYAN "create", stdout);
			break;
			case ID_READ:
				fputs_unlocked(ANSI_MAGENTA "read", stdout);
			break;
			case ID_WRITE:
				fputs_unlocked(ANSI_YELLOW "write", stdout);
			break;
			case ID_DESTROY:
				fputs_unlocked(ANSI_RED "destroy", stdout);
			break;
			case ID_CPUOP:
				fputs_unlocked(ANSI_GREEN "cpu op", stdout);
			break;
		}

		if (e.id == ID_CREATE) {
			printf(ANSI_RESET " buffer %s%u%s at %s%u%s ms (%s%u%s bytes%s)\n",
				NUMBER_HL, e.buffer, ANSI_RESET,
				NUMBER_HL, e.time, ANSI_RESET,
				NUMBER_HL, e.size, ANSI_RESET,
				e.high_prio ? ", " ANSI_YELLOW "high priority" ANSI_RESET : "");
		} else {
			printf(ANSI_RESET " buffer %s%u%s at %s%u%s ms\n",
				NUMBER_HL, e.buffer, ANSI_RESET, NUMBER_HL,
				e.time, ANSI_RESET);
		}
	}

	fflush(stdout);
}

int main(int argc, char **argv) {

	if (argc < 2)
		die("Usage: %s file.bin\n", argv[0]);

	void * const f = gzopen(argv[1], "rb");
	if (!f) die("Failed to open file\n");

	struct stat st;
	stat(argv[1], &st);

	char magic[MAGICLEN];
	sgzread(magic, MAGICLEN, f);

	if (memcmp(magic, MAGIC, MAGICLEN))
		die("This is not a bostats binary file.\n");

	u32 buffers;
	sgzread(&buffers, 4, f);

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

	u8 charbuf = 0;
	if (buffers < UCHAR_MAX)
		charbuf = 1;
	else if (buffers < USHRT_MAX)
		charbuf = 2;

	go(f, st.st_size, charbuf);

	fclose(stdout);
	if (p)
		pclose(p);
	gzclose(f);
	return 0;
}
