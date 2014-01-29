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
#include "vram.h"
#include "helpers.h"
#include "header.h"
#include "neural.h"
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

static void usage(const char name[]) {
	die("Usage: %s\n\n"
		"Modes:\n"
		"	-b --benchmark	Test the current state vs LRU (default)\n"
		"	-r --revolve	Revolutions\n"
		"	-e --evolve	Evolutions\n"
		"	-f --finetune	Fine tuning\n\n"
		"Options:\n"
		"	-v --vram 64	Only test this vram amount\n"
		, name);
}

static u8 quit = 0;

static void signaller(int num __attribute__((unused))) {
	quit = 1;
}

static void printscore(const u64 old, const u64 new) {
	const u64 diff = (new - old) * 100;
	const float percent = ((float) diff) / old;

	printf("Score went from %lu to %lu - %f improvement\n",
		old, new, percent);
}

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

		// Some traces have use-after-free.
		// Filter those so we don't trip up.
		if (destroyed[e.buffer])
			continue;

		if (e.id == ID_CREATE) {
			// Abort the trace if it tries to create a buffer bigger than vram
			if (e.size >= vram)
				return;

			pos += 4;

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

static void simulate(const u32 edge, const u32 datafiles,
			const struct dirent **namelist,
			const struct network * const net,
			u64 scores[vramelements]) {

	u32 i, v;
	for (v = 0; v < vramelements; v++) {
		fprintf(stderr, "VRAM size %lu\n", vramsizes[v]);

		for (i = 0; i < datafiles; i++) {
			fprintf(stderr, "\tChecking file %u/%u: %s\n", i + 1, datafiles,
				namelist[i]->d_name);

			void * const f = gzbinopen(namelist[i]->d_name);

			u32 buffers;
			sgzread((char *) &buffers, 4, f);

			const u8 charbuf = getcharbuf(buffers);
			initvram(vramsizes[v] * 1024 * 1024, edge * 1024,
					buffers, net);

			destroyed = xcalloc(buffers);

			// Cache it
			char *cache;
			u32 cachelen;
			bincache(f, &cache, &cachelen);

			go(cache, cachelen, charbuf, vramsizes[v] * 1024 * 1024);

			free(destroyed);

			gzclose(f);
			scores[v] = freevram();
			free(cache);
		}
	}
}

int main(int argc, char **argv) {

	const struct option opts[] = {
		{"benchmark", 0, 0, 'b'},
		{"revolve", 0, 0, 'r'},
		{"evolve", 0, 0, 'e'},
		{"finetune", 0, 0, 'f'},
		{"help", 0, 0, 'h'},
		{"vram", 1, 0, 'v'},
		{0, 0, 0, 0}
	};

	const char optstr[] = "brefv:h";

	enum {
		BENCH = 0,
		REVOLVE,
		EVOLVE,
		FINETUNE
	} mode = BENCH;

	u32 onlyvram = 0;

	while (1) {
		int c = getopt_long(argc, argv, optstr, opts, NULL);
		if (c == -1) break;

		switch (c) {
			case 'b':
				mode = BENCH;
			break;
			case 'r':
				mode = REVOLVE;
			break;
			case 'e':
				mode = EVOLVE;
			break;
			case 'f':
				mode = FINETUNE;
			break;
			case 'v':
				onlyvram = atoi(optarg);
				if (onlyvram < 64 || onlyvram > 16384)
					die("VRAM %u outside of usable limits\n",
						onlyvram);
			break;
			case 'h':
			default:
				usage(argv[0]);
		}
	}
	if (optind != argc)
		usage(argv[0]);

	signal(SIGINT, signaller);

	const char *datadir = NULL;

	// Locate the data files
	if (!access("INDEX", R_OK))
		datadir = ".";
	else if (!access("data/INDEX", R_OK))
		datadir = "data";
	else if (!access("../data/INDEX", R_OK))
		datadir = "../data";

	if (!datadir) die("Cannot find data\n");

	// Loop over the data files
	struct dirent **namelist;
	int datafiles = scandir(datadir, &namelist, filterdata, alphasort);
	if (datafiles < 0) die("Failed in scandir\n");

	chdir(datadir);

	// Read up current values
	struct network ai, lastai;
	readhdr(&ai);
	lastai = ai;

	// Do baseline simulation
	u64 basescores[vramelements], scores[vramelements];

	if (mode == BENCH)
		return 0;

	u32 iters = 0;
	u8 improved = 0;
	while (!quit) {
		ai = lastai;
		// Mutate

		// Test
		usleep(1);
		iters++;

		// Did it improve?
		if (1) {
			improved = 1;
			lastai = ai;
		}
	}

	printf("Ran for %u iterations.\n", iters);

	if (improved) {
		// Print results, save new file
		writehdr(&ai);
	} else {
		printf("No improvement was found.\n");
	}

	// Cleanup
	u32 i;
	for (i = 0; i < (u32) datafiles; i++) {
		free(namelist[i]);
	}
	free(namelist);

	return 0;
}
