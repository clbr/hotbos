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

	// Read up current values
	struct network ai, lastai;
	readhdr(&ai);
	lastai = ai;

	// Do baseline simulation

	if (mode == BENCH) {
		return 0;
	}

	u32 iters = 0;
	u8 improved = 0;
	while (!quit) {
		usleep(1);
		iters++;
	}

	printf("Ran for %u iterations.\n", iters);

	if (improved) {
		// Print results, save new file
		writehdr(&ai);
	} else {
		printf("No improvement was found.\n");
	}

	return 0;
}
