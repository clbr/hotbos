#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include "lrtypes.h"

static void tanhtest(const u32 rounds) {

}

static void smoothertest(const u32 rounds) {

}

static void results(const struct timeval * const start,
			const struct timeval * const end) {

	u32 us = end->tv_usec - start->tv_usec;
	us += (end->tv_sec - start->tv_sec) * 1000 * 1000;

	printf("%u us (%u ms)\n\n", us, us / 1000);
}

int main() {

	const u32 rounds = 1000 * 1000 * 1000;
	struct timeval start, end;

	printf("Testing tanh, %u rounds...\n", rounds);
	gettimeofday(&start, NULL);
	tanhtest(rounds);
	gettimeofday(&end, NULL);

	results(&start, &end);

	printf("Testing smootherstep, %u rounds...\n", rounds);
	gettimeofday(&start, NULL);
	smoothertest(rounds);
	gettimeofday(&end, NULL);

	results(&start, &end);

	return 0;
}
