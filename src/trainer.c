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
#include <time.h>
#include <fcntl.h>

static void usage(const char name[]) {
	die("Usage: %s\n\n"
		"Modes:\n"
		"	-b --benchmark	Test the current state vs LRU (default)\n"
		"	-r --revolve	Revolutions\n"
		"	-e --evolve	Evolutions\n"
		"	-f --finetune	Fine tuning\n\n"
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

static u64 sumscore(const u64 arr[vramelements]) {

	u64 sum = 0;
	u32 i;
	for (i = 0; i < vramelements; i++) {
		sum += arr[i];
	}

	return sum;
}

static void printscores(const u64 olds[vramelements], const u64 news[vramelements]) {
	u32 i;
	for (i = 0; i < vramelements; i++) {
		printf("%lu: ", vramsizes[i]);
		printscore(olds[i], news[i]);
	}

	printf("\nTotal: ");
	printscore(sumscore(olds), sumscore(news));
}

static int acceptable(const u64 olds[vramelements], const u64 news[vramelements]) {

	const u64 oldsum = sumscore(olds);
	const u64 newsum = sumscore(news);

	if (newsum >= oldsum)
		return 0;

	u32 i;
	for (i = 0; i < vramelements; i++) {
		if (news[i] > olds[i]) {
			// If a case got worse, by how much? 5% is acceptable
			const u64 diff = (news[i] - olds[i]) * 100;
			const u64 per = diff / olds[i];

			if (per >= 5)
				return 0;
		}
	}

	return 1;
}

static u8 *destroyed;

static void go(void * const f, const u32 size, const u8 charbufs, const u64 vram) {

	entry e;
	const u8 cb = charbufs ? charbufs : 3;
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
	}

	fflush(stdout);
}

static void simulate(const u32 edge, const u32 datafiles,
			struct dirent * const * const namelist,
			const struct network * const net,
			u64 scores[vramelements]) {

	u32 i, v;
	for (v = 0; v < vramelements; v++) {
		scores[v] = 0;
		for (i = 0; i < datafiles; i++) {
			printf("\r\tVRAM %lu: Checking file %u/%u: %s"
				"                               ", vramsizes[v],
				i + 1, datafiles,
				namelist[i]->d_name);
			fflush(stdout);

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
			scores[v] += freevram();
			free(cache);
		}
	}

	printf("\r                                                  ");
	fflush(stdout);
}

int main(int argc, char **argv) {

	srand(time(NULL));

	const struct option opts[] = {
		{"benchmark", 0, 0, 'b'},
		{"revolve", 0, 0, 'r'},
		{"evolve", 0, 0, 'e'},
		{"finetune", 0, 0, 'f'},
		{"help", 0, 0, 'h'},
		{0, 0, 0, 0}
	};

	const char optstr[] = "brefh";

	enum {
		BENCH = 0,
		REVOLVE,
		EVOLVE,
		FINETUNE
	} mode = BENCH;

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

	// Read up current values
	struct network ai, lastai;
	readhdr(&ai);
	lastai = ai;

	const int pwd = open(".", O_RDONLY);
	if (pwd < 0) die("Failed to get current dir\n");
	chdir(datadir);

	// Do baseline simulation
	u64 basescores[vramelements], scores[vramelements], lastscores[vramelements];
	printf("Doing baseline simulation.\n");

	simulate(0, datafiles, namelist, NULL, basescores);
	simulate(512, datafiles, namelist, &ai, scores);

	printscores(basescores, scores);

	if (mode == BENCH)
		return 0;

	u32 iters = 0;
	u8 improved = 0;
	memcpy(lastscores, scores, sizeof(u64) * vramelements);
	while (!quit) {
		ai = lastai;
		// Mutate

		// Test
		simulate(512, datafiles, namelist, &ai, scores);
		iters++;

		// Did it improve?
		if (acceptable(lastscores, scores)) {
			improved = 1;
			lastai = ai;
			memcpy(lastscores, scores, sizeof(u64) * vramelements);
		}
	}

	printf("Ran for %u iterations.\n", iters);

	if (improved) {
		// Print results, save new file
		printf("Improvement:\n");
		printscores(basescores, lastscores);

		fchdir(pwd);
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

	close(pwd);

	return 0;
}
