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

struct buf {
	struct buf *next;
	struct buf *prev;
	u64 tick;
	u64 size;
	u32 id;
	u8 hole;
	u8 vram;
};

static struct {
	u64 size;
	u64 tick;
	u32 edge;

	struct buf *vram;
	struct buf *ram;

	struct buf *storage;
} ctx;

void initvram(const u64 size, const u32 edge, const u32 buffers) {

	ctx.size = size;
	ctx.edge = edge;
	ctx.tick = 0;

	ctx.vram = xcalloc(sizeof(struct buf));
	ctx.vram->size = size;
	ctx.vram->hole = 1;

	ctx.storage = xcalloc(buffers * sizeof(struct buf));
}

void freevram() {

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
}

static void dropvrambuf(struct buf * const oldest) {

	oldest->vram = 0;

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

		if (hole->prev)
			hole->prev->next = hole;
		else if (oldest != ctx.vram)
			die("%p had no prev, but is not vram\n", oldest);

		if (hole->next)
			hole->next->prev = hole;

		if (oldest == ctx.vram)
			ctx.vram = hole;
		return;
	}

	if (oldest == ctx.vram)
		ctx.vram = oldest->next;
}

static void dropoldest() {

	printf("Fragmentation caused a swap\n");

	struct buf *cur, *oldest;

	oldest = cur = ctx.vram;
	while (cur) {
		if (cur->hole) {
			cur = cur->next;
			continue;
		}

		if (oldest->hole)
			oldest = cur;

		if (cur->tick < oldest->tick)
			oldest = cur;

		cur = cur->next;
	}

	if (oldest->hole) die("Tried to drop a hole\n");

	dropvrambuf(oldest);

	// Drop oldest
	oldest->prev = NULL;
	oldest->next = ctx.ram;
	ctx.ram->prev = oldest;
	ctx.ram = oldest;
}

static struct buf *fits(const u32 size) {

	if (!ctx.edge || size < ctx.edge) {
		// From the beginning
		struct buf *cur = ctx.vram;
		while (cur) {
			if (!cur->hole) {
				cur = cur->next;
				continue;
			}

			if (cur->size > size) {
				return cur;
			}

			cur = cur->next;
		}
	} else {
		// From the end
		struct buf *cur = ctx.vram;
		while (cur->next)
			cur = cur->next;

		while (cur) {
			if (!cur->hole) {
				cur = cur->prev;
				continue;
			}

			if (cur->size > size) {
				return cur;
			}

			cur = cur->prev;
		}
	}

	return NULL;
}

void destroybuf(const u32 id) {

	// Is it in ram? Easy drop then
	struct buf *cur = ctx.ram;
	while (cur) {
		if (cur->id == id) {
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

		cur = cur->next;
	}

	// Nope, it's in vram then
	cur = ctx.vram;
	u8 found = 0;
	while (cur) {
		if (cur->id == id) {
			found = 1;
			break;
		}

		cur = cur->next;
	}

	if (!found) die("Tried to drop a buffer that doesn't exist\n");

	dropvrambuf(cur);
	if (cur->hole)
		free(cur);
}

static void internaltouch(const u32 id) {

	// The meat.

	// Find the buffer in RAM
	struct buf *cur = ctx.ram;
	u8 found = 0;
	while (cur) {
		if (cur->id == id) {
			found = 1;
			break;
		}

		cur = cur->next;
	}

	if (!found) die("Asked to find a buffer not in RAM %u\n", id);

	cur->tick = ctx.tick;
	cur->vram = 1;

	// Check if there's space for it
	struct buf *const mine = cur;
	struct buf *fit = fits(mine->size);

	while (!fit) {
		dropoldest();
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

void allocbuf(const u32 id, const u32 size) {

	ctx.tick++;

	struct buf *cur = &ctx.storage[id];
	cur->size = size;
	cur->id = id;
	cur->tick = ctx.tick;

	// Allocate a new buffer, put it to RAM, internaltouch moves it to vram
	cur->next = ctx.ram;
	if (ctx.ram)
		ctx.ram->prev = cur;
	ctx.ram = cur;

	internaltouch(id);
}

void touchbuf(const u32 id) {

	ctx.tick++;

	// Is the buffer in VRAM? If so, update its timestamp and quit
	if (ctx.storage[id].vram) {
		ctx.storage[id].tick = ctx.tick;
		return;
	}

	// It's not. Touch it from RAM.
	internaltouch(id);
}

void checkfragmentation() {

	u32 total = 0;
	u64 totalsize = 0;
	struct buf *cur = ctx.vram;

	while (cur) {
		if (cur->hole)
			total++;

		totalsize += cur->size;

		cur = cur->next;
	}

	total--;

	printf("Fragments: %u\n", total);

	if (totalsize != ctx.size)
		die("VRAM got corrupted, size doesn't match (%llu s/b %llu)\n",
			(unsigned long long) totalsize,
			(unsigned long long) ctx.size);
}
