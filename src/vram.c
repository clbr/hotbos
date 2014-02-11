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

#include "vram.h"
#include "helpers.h"
#include "scoring.h"
#include "bucket.h"

struct buf {
	struct {
		u64 reads;
		u64 writes;
		u64 lastread;
		u64 lastwrite;
		u64 cpuops;
		u64 lastcpu;
	} stats;
	struct buf *next;
	struct buf *prev;
	u64 tick;
	u64 size;
	u32 id;
	u32 score;
	u8 hole;
	u8 vram;
	u8 highprio;
};

static struct {
	u64 size;
	u64 tick;
	u32 edge;
	u32 holes;
	u64 score;

	struct buf *vram;
	struct buf *ram;

	struct buf *storage;
	struct buf **holelist;

	struct bucket *bucket;

	const struct network *net;
} ctx;

void initvram(const u64 size, const u32 edge, const u32 buffers,
		const struct network * const net) {

	ctx.size = size;
	ctx.edge = edge;
	ctx.tick = 0;
	ctx.holes = 1;
	ctx.net = net;
	ctx.score = 0;

	ctx.vram = xcalloc(sizeof(struct buf));
	ctx.vram->size = size;
	ctx.vram->hole = 1;

	ctx.storage = xcalloc(buffers * sizeof(struct buf));
	ctx.holelist = xcalloc((buffers + 2) * sizeof(void *));
	ctx.holelist[0] = ctx.vram;

	ctx.bucket = initbuckets(buffers);
}

static float clampf(const float in, const float min, const float max) {
	return in > max ? max : in < min ? min : in;
}

static void stats2inputs(const struct buf * const in, float inputs[INPUT_NEURONS]) {

	inputs[0] = clampf(in->size / (1024.0f * 1024 * 1024), 0, 1);
	inputs[1] = clampf(ctx.size / (1024.0f * 1024 * 1024 * 10), 0, 1);
	inputs[2] = in->highprio;
	inputs[3] = clampf((ctx.tick - in->stats.lastread) / 2400.0f, 0, 1);
	inputs[4] = clampf((ctx.tick - in->stats.lastwrite) / 2400.0f, 0, 1);
	inputs[5] = clampf(in->stats.reads / 500.0f, 0, 1);
	inputs[6] = clampf(in->stats.writes / 500.0f, 0, 1);
	inputs[7] = clampf((ctx.tick - in->stats.lastcpu) / 2400.0f, 0, 1);
	inputs[8] = clampf(in->stats.cpuops / 500.0f, 0, 1);
}

static void genholelist() {

	struct buf *cur = ctx.vram;
	u32 num = 0;
	while (cur) {
		if (cur->hole) {
			ctx.holelist[num] = cur;

			num++;

			// Early exit if we found all holes
			if (num == ctx.holes)
				break;
		}

		cur = cur->next;
	}
}

static void dropholefromlist(const struct buf * const in) {

	// Locate it
	u32 i;
	for (i = 0; i < ctx.holes; i++) {
		if (ctx.holelist[i] == in) {
			if (i == ctx.holes - 1)
				return;

			const u32 amount = ctx.holes - i - 1;
			memmove(&ctx.holelist[i], &ctx.holelist[i + 1], amount * sizeof(void *));

			return;
		}
	}

	die("Didn't find the hole to drop?\n");
}

u64 freevram() {

	ctx.size = ctx.edge = 0;

	struct buf *cur = ctx.vram;
	while (cur) {
		struct buf *next = cur->next;
		if (cur->hole)
			free(cur);
		cur = next;
	}

	cur = ctx.ram;
	while (cur) {
		struct buf *next = cur->next;
		if (cur->hole)
			free(cur);
		cur = next;
	}

	ctx.vram = ctx.ram = NULL;

	free(ctx.storage);
	ctx.storage = NULL;
	free(ctx.holelist);
	ctx.holelist = NULL;

	ctx.net = NULL;

	freebuckets(ctx.bucket);

	return ctx.score;
}

static void dropvrambuf(struct buf * const oldest) {

	oldest->vram = 0;

	ctx.score += score(SCORE_GPU, SCORE_MOVE, SCORE_RAM, oldest->size);

	// Is the buffer on either side a hole? If so, merge
	if (oldest->prev && oldest->next && oldest->prev->hole && oldest->next->hole) {
		// Merge two holes
		struct buf *hole1 = oldest->prev;
		struct buf *hole2 = oldest->next;

		hole1->size += oldest->size + hole2->size;
		hole1->next = hole2->next;
		if (hole1->next)
			hole1->next->prev = hole1;

		free(hole2);

		dropholefromlist(hole2);
		ctx.holes--;

	} else if (oldest->prev && oldest->prev->hole) {
		struct buf *hole = oldest->prev;
		hole->size += oldest->size;
		hole->next = oldest->next;

		if (hole->next)
			hole->next->prev = hole;

	} else if (oldest->next && oldest->next->hole) {
		struct buf *hole = oldest->next;
		hole->size += oldest->size;
		hole->prev = oldest->prev;

		if (hole->prev)
			hole->prev->next = hole;
	} else {
		// No hole on either side, we need to allocate a new hole
		struct buf * const hole = xcalloc(sizeof(struct buf));
		hole->hole = 1;
		hole->size = oldest->size;

		hole->prev = oldest->prev;
		hole->next = oldest->next;

		ctx.holes++;

		if (hole->prev)
			hole->prev->next = hole;
		else if (oldest != ctx.vram)
			die("%p had no prev, but is not vram\n", oldest);

		if (hole->next)
			hole->next->prev = hole;

		if (oldest == ctx.vram)
			ctx.vram = hole;

		genholelist();
		return;
	}

	if (oldest == ctx.vram)
		ctx.vram = oldest->next;
}

static struct buf *findoldest() {

	struct buf *cur, *oldest;

	oldest = cur = ctx.vram;
	while (cur) {
		if (cur->hole) {
			cur = cur->next;
			continue;
		}

		if (oldest->hole)
			oldest = cur;

		if (ctx.net) {
			if (cur->score < oldest->score)
				oldest = cur;
		} else {
			if (cur->tick < oldest->tick)
				oldest = cur;
		}

		cur = cur->next;
	}

	if (oldest->hole) die("Tried to drop a hole\n");

	return oldest;
}

static void dropoldest(struct buf *oldest) {

	//printf("Fragmentation caused a swap\n");

	if (!oldest)
		oldest = findoldest();

	dropvrambuf(oldest);

	// Drop oldest
	oldest->prev = NULL;
	oldest->next = ctx.ram;
	ctx.ram->prev = oldest;
	ctx.ram = oldest;
}

static struct buf *fits(const u32 size) {

	const u32 max = ctx.holes;

	if (!ctx.edge || size < ctx.edge) {
		// From the beginning
		u32 i;
		for (i = 0; i < max; i++) {
			if (ctx.holelist[i]->size > size)
				return ctx.holelist[i];
		}
	} else {
		// From the end
		s32 i;
		for (i = max - 1; i >= 0; i--) {
			if (ctx.holelist[i]->size > size)
				return ctx.holelist[i];
		}
	}

	return NULL;
}

void destroybuf(const u32 id) {

	// Is it in ram? Easy drop then
	struct buf * const cur = &ctx.storage[id];
	if (!cur->vram) {
		if (cur->prev)
			cur->prev->next = cur->next;
		if (cur->next)
			cur->next->prev = cur->prev;
		if (cur == ctx.ram)
			ctx.ram = cur->next;

		if (cur->hole)
			free(cur);
		return;
	}

	// Nope, it's in vram then
	dropvrambuf(cur);
	if (cur->hole)
		free(cur);
}

static void internaltouch(const u32 id, const u8 write) {

	// The meat.
	struct buf *cur = &ctx.storage[id];

	cur->tick = ctx.tick;

	// Check if there's space for it
	struct buf *const mine = cur;
	struct buf *fit = fits(mine->size);

	if (!fit && ctx.net) {
		// Check whether we should move it to VRAM at all
		struct buf * const oldest = findoldest();
		if (oldest->score > cur->score) {
			ctx.score += score(SCORE_GPU, write ? SCORE_W : SCORE_R, SCORE_RAM,
					ctx.storage[id].size);
			return;
		}

		// Since we know the oldest one, drop it now
		dropoldest(oldest);
		fit = fits(mine->size);
	}
	ctx.score += score(SCORE_GPU, SCORE_MOVE, SCORE_VRAM, ctx.storage[id].size);
	cur->vram = 1;

	while (!fit) {
		dropoldest(NULL);
		fit = fits(mine->size);
	}

	// Cool, it fits in the space occupied by this hole
	fit->size -= mine->size;
	if (mine->next)
		mine->next->prev = mine->prev;
	if (mine->prev)
		mine->prev->next = mine->next;
	if (mine == ctx.ram)
		ctx.ram = mine->next;

	// Do we add it to its start or end?
	if (!ctx.edge || mine->size < ctx.edge) {
		// start
		mine->prev = fit->prev;
		mine->next = fit;

		if (fit->prev)
			fit->prev->next = mine;
		fit->prev = mine;

		if (ctx.vram == fit)
			ctx.vram = mine;
	} else {
		// end
		mine->prev = fit;
		mine->next = fit->next;

		if (fit->next)
			fit->next->prev = mine;
		fit->next = mine;
	}
}

void allocbuf(const u32 id, const u32 size, const u8 highprio) {

	ctx.tick++;

	struct buf *cur = &ctx.storage[id];
	cur->size = size;
	cur->id = id;
	cur->tick = ctx.tick;
	cur->highprio = highprio;

	// Allocate a new buffer, put it to RAM, internaltouch moves it to vram
	cur->next = ctx.ram;
	if (ctx.ram)
		ctx.ram->prev = cur;
	ctx.ram = cur;

	internaltouch(id, 0);
}

void touchbuf(const u32 id, const u8 write) {

	ctx.tick++;

	// If in AI mode, and enough time since last update, update the buffer's score
	if (ctx.net &&
		(!ctx.storage[id].score || (ctx.tick - ctx.storage[id].tick) > 600)) {

		float inputs[INPUT_NEURONS];
		stats2inputs(&ctx.storage[id], inputs);

		ctx.storage[id].score = calculate_score(inputs, ctx.net);
	}

	// Update its stats
	if (write) {
		ctx.storage[id].stats.lastwrite = ctx.tick;
		ctx.storage[id].stats.writes++;
	} else {
		ctx.storage[id].stats.lastread = ctx.tick;
		ctx.storage[id].stats.reads++;
	}

	// Is the buffer in VRAM? If so, update its timestamp and quit
	if (ctx.storage[id].vram) {
		ctx.storage[id].tick = ctx.tick;

		ctx.score += score(SCORE_GPU, write ? SCORE_W : SCORE_R, SCORE_VRAM,
					ctx.storage[id].size);
		return;
	}

	// It's not. Touch it from RAM.
	internaltouch(id, write);
}

void cpubuf(const u32 id) {
	u8 vram = SCORE_RAM;
	if (ctx.storage[id].vram)
		vram = SCORE_VRAM;

	ctx.score += score(SCORE_CPU, SCORE_W, vram, ctx.storage[id].size);

	ctx.storage[id].stats.cpuops++;
	ctx.storage[id].stats.lastcpu = ctx.tick;
}

void checkfragmentation() {

	u32 total = 0;
	total = ctx.holes;

	/*
	u64 totalsize = 0;
	struct buf *cur = ctx.vram;
	while (cur) {
		if (cur->hole)
			total++;

		totalsize += cur->size;

		cur = cur->next;
	}*/

	total--;

	printf("Fragments: %u\n", total);
/*
	if (totalsize != ctx.size)
		die("VRAM got corrupted, size doesn't match (%llu s/b %llu)\n",
			(unsigned long long) totalsize,
			(unsigned long long) ctx.size);*/
}
