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
#include "fragvram.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

static u8 *destroyed;

static void go(void * const f, const u32 size, const u8 charbufs, const u64 vram) {

	entry e;
	const u8 cb = charbufs ? charbufs : 3;
	u8 ctr = 0;
	u32 ctx = 0;
	u32 pos = 0;

	while (pos < size) {

		readentry(&e, f + pos, cb, &ctx);
		pos += cb + 1;

		if (e.id == ID_CPUOP)
			continue;
		if (e.id == ID_CREATE)
			pos += 4;

		// Some traces have use-after-free.
		// Filter those so we don't trip up.
		if (destroyed[e.buffer])
			continue;

		if (e.id == ID_CREATE) {
			// Abort the trace if it tries to create a buffer bigger than vram
			if (e.size >= vram)
				return;

			allocbuf(e.buffer, e.size);
		} else if (e.id == ID_DESTROY) {
			destroybuf(e.buffer);

			destroyed[e.buffer] = 1;
		} else {
			touchbuf(e.buffer);
		}

		if (!ctr)
			checkfragmentation();

		ctr++;
		ctr %= 10;
	}

	fflush(stdout);
}

int main(int argc, char **argv) {

	if (argc > 1 && argv[1][0] == '-') {
		die("Usage: %s [optional strategy]\n\n"
			"Strategies: default, minmax (number)\n",
			argv[0]);
	}

	const char *datadir = NULL;

	// Locate the data files
	if (!access("INDEX", R_OK))
		datadir = ".";
	else if (!access("data/INDEX", R_OK))
		datadir = "data";
	else if (!access("../data/INDEX", R_OK))
		datadir = "../data";

	if (!datadir) die("Cannot find data\n");

	// Determine strategy
	u32 edge = 0;
	if (!argv[1] || !strcmp(argv[1], "default")) {
		fprintf(stderr, "Measuring default strategy.\n");
	} else if (argc > 2 && !strcmp(argv[1], "minmax")) {
		edge = atoi(argv[2]);
		fprintf(stderr, "Measuring minmax strategy, with edge %u kb.\n",
			edge);
	} else {
		die("Unknown strategy\n");
	}

	// Loop over the data files
	struct dirent **namelist;
	int datafiles = scandir(datadir, &namelist, filterdata, alphasort);
	if (datafiles < 0) die("Failed in scandir\n");

	chdir(datadir);

	u32 i, v;
	for (v = 0; v < vramelements; v++) {
		fprintf(stderr, "VRAM size %lu\n", vramsizes[v]);
		printf("------------------------ VRAM size %lu\n", vramsizes[v]);

		for (i = 0; i < (u32) datafiles; i++) {
			fprintf(stderr, "\tChecking file %u/%u: %s\n", i + 1, datafiles,
				namelist[i]->d_name);

			void * const f = gzbinopen(namelist[i]->d_name);

			u32 buffers;
			sgzread(&buffers, 4, f);

			const u8 charbuf = getcharbuf(buffers);
			initvram(vramsizes[v] * 1024 * 1024, edge * 1024,
					buffers);

			destroyed = xcalloc(buffers);

			// Cache it
			char *cache;
			u32 cachelen;
			bincache(f, &cache, &cachelen);

			go(cache, cachelen, charbuf, vramsizes[v] * 1024 * 1024);

			free(destroyed);

			gzclose(f);
			freevram();
			free(cache);
		}
	}

	for (i = 0; i < (u32) datafiles; i++) {
		free(namelist[i]);
	}
	free(namelist);

	return 0;
}
