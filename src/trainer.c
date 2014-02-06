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

#define ERASE "\33[2K"

struct critter {
	u64 score;
	u8 genome[NEURAL_VARS];
};

static void usage(const char name[]) {
	die("Usage: %s\n\n"
		"Modes:\n"
		"	-b --benchmark	Test the current state vs LRU (default)\n"
		"	-r --revolve	Revolutions\n"
		"	-e --evolve	Evolutions\n"
		"	-f --finetune	Fine tuning\n"
		"	-g --genetic	Genetic evolution\n"
		"\n"
		, name);
}

static u8 quit = 0;

static void signaller(int num __attribute__((unused))) {

	if (quit)
		die("OK, you want out\n");

	quit = 1;
}

static void printscore(const u64 old, const u64 new) {
	const float diff = (((double) old) - new) * 100;
	const float percent = diff / old;

	if (old >= new)
		printf("Score went from %lu to %lu - %.3g%% improvement\n",
			old, new, percent);
	else
		printf("Score went from %lu to %lu - %.3g%% worse\n",
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
		printf("%5lu: ", vramsizes[i]);
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

static void go(const void * const f, const u32 size, const u8 charbufs, const u64 vram) {

	entry e;
	const u8 cb = charbufs ? charbufs : 3;
	u32 ctx = 0;
	u32 pos = 0;

	while (pos < size) {

		readentry(&e, f + pos, cb, &ctx);
		pos += cb + 1;

		if (e.id == ID_CREATE)
			pos += 4;

		// Some traces have use-after-free.
		// Filter those so we don't trip up.
		if (destroyed[e.buffer])
			continue;

		if (e.id == ID_CPUOP) {
			cpubuf(e.buffer);
			continue;
		}

		if (e.id == ID_CREATE) {
			// Abort the trace if it tries to create a buffer bigger than vram
			if (e.size >= vram)
				return;

			allocbuf(e.buffer, e.size, e.high_prio);
		} else if (e.id == ID_DESTROY) {
			destroybuf(e.buffer);

			destroyed[e.buffer] = 1;
		} else {
			touchbuf(e.buffer, e.id == ID_WRITE);
		}
	}

	fflush(stdout);
}

static void **cachedbin = NULL;
static u32 *cachedsizes = NULL;

static void simulate(const u32 edge, const u32 datafiles,
			struct dirent * const * const namelist,
			const struct network * const net,
			u64 scores[vramelements]) {

	u32 i, v;
	for (v = 0; v < vramelements; v++) {
		scores[v] = 0;
		for (i = 0; i < datafiles; i++) {
			printf(ERASE "\r\tVRAM %lu: Checking file %u/%u: %s",
				vramsizes[v],
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

			// Cache it; is it already cached?
			char *cache;
			u32 cachelen;
			if (!cachedbin[i]) {
				bincache(f, &cache, &cachelen);
			} else {
				cache = cachedbin[i];
				cachelen = cachedsizes[i];
			}

			go(cache, cachelen, charbuf, vramsizes[v] * 1024 * 1024);

			free(destroyed);

			gzclose(f);
			scores[v] += freevram();

			// Should we cache it for future runs, or free it?
			// Cache the big ones, they have the most zlib hit
			// Can't cache everything, it would take > 11gb,
			// I don't have that much RAM...
			if (!cachedbin[i]) {
				if (cachelen >= 300 * 1024 * 1024) {
					cachedbin[i] = cache;
					cachedsizes[i] = cachelen;
				} else {
					free(cache);
				}
			}
		}
	}

	printf(ERASE "\r");
	fflush(stdout);
}

static void mutate(struct network * const ai, const float minchange,
			const u32 change, const u32 targets) {
	u32 i;
	for (i = 0; i < targets; i++) {
		const u32 layer = rand() % 3;
		const u32 neuron = rand() % INPUT_NEURONS;
		const u32 weight = rand() % INPUT_NEURONS;
		const u32 bias = rand() % 4 == 0;
		float *val = NULL;

		switch (layer) {
			case 0:
				if (bias)
					val = &ai->input[neuron].bias;
				else
					val = &ai->input[neuron].weight;
			break;
			case 1:
				if (bias)
					val = &ai->hidden[neuron].bias;
				else
					val = &ai->hidden[neuron].weights[weight];
			break;
			case 2:
				if (bias)
					val = &ai->output.bias;
				else
					val = &ai->output.weights[weight];
			break;
		}

		float per = rand() % (change * 10);
		per /= 1000.0f;

		float amount = *val * per;
		if (amount < minchange)
			amount = minchange;

		// Plus or minus?
		if (rand() % 2)
			*val -= amount;
		else
			*val += amount;
	}
}

static float gene2f(const u8 gene) {
	float out = gene / 127.5f - 1;
	return out;
}

static void genome2ai(const u8 genome[NEURAL_VARS], struct network * const ai) {
	u32 i, g = 0;

	for (i = 0; i < INPUT_NEURONS; i++) {
		ai->input[i].weight = gene2f(genome[g++]);
		ai->input[i].bias = gene2f(genome[g++]);
	}

	for (i = 0; i < INPUT_NEURONS; i++) {
		u32 w;
		for (w = 0; w < INPUT_NEURONS; w++)
			ai->hidden[i].weights[w] = gene2f(genome[g++]);

		ai->hidden[i].bias = gene2f(genome[g++]);
	}

	for (i = 0; i < INPUT_NEURONS; i++) {
		ai->output.weights[i] = gene2f(genome[g++]);
	}
	ai->output.bias = gene2f(genome[g++]);

	if (g != NEURAL_VARS) die("genome2ai\n");
}

static int crittercmp(const void * const ap, const void * const bp) {
	const struct critter * const a = (struct critter *) ap;
	const struct critter * const b = (struct critter *) bp;

	if (a->score < b->score)
		return -1;
	else if (a->score > b->score)
		return 1;

	return 0;
}

int main(int argc, char **argv) {

	srand(time(NULL));

	const struct option opts[] = {
		{"benchmark", 0, 0, 'b'},
		{"revolve", 0, 0, 'r'},
		{"evolve", 0, 0, 'e'},
		{"finetune", 0, 0, 'f'},
		{"help", 0, 0, 'h'},
		{"genetic", 0, 0, 'g'},
		{0, 0, 0, 0}
	};

	const char optstr[] = "brefhg";

	enum {
		BENCH = 0,
		REVOLVE,
		EVOLVE,
		FINETUNE,
		GENETIC
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
			case 'g':
				mode = GENETIC;
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

	cachedbin = xcalloc(datafiles * sizeof(void *));
	cachedsizes = xcalloc(datafiles * sizeof(u32));

	// Do baseline simulation
	u64 basescores[vramelements], scores[vramelements], lastscores[vramelements];
	printf("Doing baseline simulation.\n");

	// Pre-calculated base scores
	basescores[0] =	11373443721800562ULL;
	basescores[1] = 8457631614472705ULL;
	basescores[2] = 6823682679644306ULL;
	basescores[3] = 6366782903587257ULL;
	basescores[4] = 6103944126818047ULL;
	basescores[5] = 5889442175382210ULL;
	basescores[6] = 6091687353267279ULL;
	basescores[7] = 6788258883170870ULL;
	basescores[8] = 6768448221629419ULL;

//	if (mode == BENCH)
//		simulate(0, datafiles, namelist, NULL, basescores);
	if (mode != GENETIC)
		simulate(512, datafiles, namelist, &ai, scores);

	if (mode == BENCH) {
		printscores(basescores, scores);
		return 0;
	}

	const u32 popmax = 10;
	struct critter pop[popmax];
	u32 i, j;
	if (mode == GENETIC) {
		for (i = 0; i < popmax; i++) {
			for (j = 0; j < NEURAL_VARS; j++) {
				pop[i].genome[j] = rand() % 256;
			}
			pop[i].score = 0;
		}
	}

	u32 iters = 0;
	u64 oldbest = ULLONG_MAX;
	u8 improved = 0;
	u8 fruitless = 0;
	memcpy(lastscores, scores, sizeof(u64) * vramelements);
	if (mode != GENETIC) while (!quit) {
		ai = lastai;
		// Mutate
		u32 change = 0, targets = 0;
		float minchange = 0;
		switch (mode) {
			case REVOLVE:
				targets = 5;
				minchange = 0.05;
				change = 20;
			break;
			case EVOLVE:
				targets = 3;
				minchange = 0.02;
				change = 5;
			break;
			case FINETUNE:
				targets = 1;
				minchange = 0.01;
				change = 3;
			break;
			default:
				die("Unknown mode\n");
		}
		mutate(&ai, minchange, change, targets);

		// Test
		simulate(512, datafiles, namelist, &ai, scores);
		iters++;

		// Did it improve?
		if (acceptable(lastscores, scores)) {
			puts("Improved!");

			improved = 1;
			lastai = ai;
			memcpy(lastscores, scores, sizeof(u64) * vramelements);
		} else {
			puts("No improvement.");
		}
	} else while (!quit) {

		// For each critter, calculate a score
		for (i = 0; i < popmax; i++) {
			genome2ai(pop[i].genome, &ai);
			simulate(512, datafiles, namelist, &ai, scores);
			pop[i].score = sumscore(scores);
		}

		// Sort them by score
		qsort(pop, popmax, sizeof(struct critter), crittercmp);

		// Kill worse half
		for (i = popmax / 2; i < popmax; i++) {
			pop[i].score = 0;
			memset(pop[i].genome, 0, NEURAL_VARS);
		}

		// Sex
		for (i = popmax / 2; i < popmax; i++) {
			// i is the target. Now who will mate?
			u32 x = 0, y = 0;
			x = (rand() % (popmax * popmax)) / popmax;
			y = (rand() % (popmax * popmax)) / popmax;
			while (x == y)
				y = (rand() % (popmax * popmax)) / popmax;

			// Swap a few genes
			for (j = 0; j < 5; j++) {
				u32 g = rand() % NEURAL_VARS;
				u8 tmp = pop[x].genome[g];
				pop[x].genome[g] = pop[y].genome[g];
				pop[y].genome[g] = tmp;
			}

			// Kid is half papa, half mama
			memcpy(pop[i].genome, pop[x].genome, NEURAL_VARS / 2);
			memcpy(pop[i].genome + NEURAL_VARS / 2,
				pop[y].genome + NEURAL_VARS / 2,
				NEURAL_VARS / 2);
		}

		// Mutations
		for (i = 0; i < popmax; i++) {
			if (rand() % 1000 == 666) {
				pop[i].genome[rand() % NEURAL_VARS] ^= rand() % 256;
			}
		}

		// Did best improve?
		if (pop[0].score < oldbest) {
			fruitless = 0;

			if (oldbest != ULLONG_MAX) {
				improved = 1;
				puts("Improved");
			}
			oldbest = pop[0].score;
		} else {
			puts("No improvement");
			fruitless++;
		}

		iters++;
		if (fruitless > 20) {
			puts("Converged");
			break;
		}
	}

	printf("Ran for %u iterations.\n", iters);

	if (improved) {
		// Print results, save new file
		printf("Improvement:\n");
		printscores(basescores, lastscores);

		fchdir(pwd);
		writehdr(&lastai);
	} else {
		printf("No improvement was found.\n");
	}

	// Cleanup
	for (i = 0; i < (u32) datafiles; i++) {
		free(namelist[i]);
		free(cachedbin[i]);
	}
	free(namelist);
	free(cachedbin);
	free(cachedsizes);

	close(pwd);

	return 0;
}
