#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include "lrtypes.h"

static const u32 scale = 1024;

static u32 fixedmul(const u32 a, const u32 b) {

	u32 out = a * b;
	out /= scale;

	return out;
}

static u32 clampu(const u32 in, const u32 min, const u32 max) {
	return in > max ? max : in < min ? min : in;
}

static float clampf(const float in, const float min, const float max) {
	return in > max ? max : in < min ? min : in;
}

static u32 smootherstep(const u32 e0, const u32 e1, u32 x) {
	x = clampu((x - e0)/(e1 - e0), 0, scale);

	u32 tmp = fixedmul(x, 6 * scale);
	tmp -= 15 * scale;
	tmp = fixedmul(tmp, x);
	tmp += 10 * scale;

	tmp = fixedmul(tmp, x);
	tmp = fixedmul(tmp, x);
	tmp = fixedmul(tmp, x);

//	return x*x*x*(x*(x*6 - 15) + 10);
	return tmp;
}

static float smootherstepf(const float e0, const float e1, float x) {
	x = clampf((x - e0)/(e1 - e0), 0, 1);

	return x*x*x*(x*(x*6 - 15) + 10);
}

static void tanhtest(const u32 rounds) {

	u32 i;
	float sum = 0;
	for (i = 0; i < rounds; i++) {
		sum += tanhf(i);
	}

	printf("sum %g\n", sum);
}

static void smoothertest(const u32 rounds) {

	u32 i;
	u32 sum = 0;
	for (i = 0; i < rounds; i++) {
		sum += smootherstep(0, 1*scale, i);
	}

	printf("sum %u\n", sum);
}

static void smootherftest(const u32 rounds) {

	u32 i;
	float sum = 0;
	for (i = 0; i < rounds; i++) {
		sum += smootherstepf(0, 1, i);
	}

	printf("sum %g\n", sum);
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


	printf("Testing smootherstep float, %u rounds...\n", rounds);
	gettimeofday(&start, NULL);
	smootherftest(rounds);
	gettimeofday(&end, NULL);

	results(&start, &end);

	return 0;
}
