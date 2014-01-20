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
	u32 id;
	u32 size;
	u8 hole;
};

static struct {
	u64 size;
	u64 tick;
	u32 edge;

	struct buf *vram;
	struct buf *ram;
} ctx;

void initvram(const u64 size, const u32 edge) {

	ctx.size = size;
	ctx.edge = edge;
	ctx.tick = 0;

	ctx.vram = xcalloc(sizeof(struct buf));
	ctx.vram->size = size;
	ctx.vram->hole = 1;
}

void freevram() {

	ctx.size = ctx.edge = 0;

	struct buf *cur = ctx.vram;
	while (cur) {
		struct buf *next = cur->next;
		free(cur);
		cur = next;
	}

	cur = ctx.ram;
	while (cur) {
		struct buf *next = cur->next;
		free(cur);
		cur = next;
	}
}

static void dropoldest() {

	struct buf *cur, *oldest;

	oldest = cur = ctx.vram;
	while (cur) {
		if (cur->hole) {
			cur = cur->next;
			continue;
		}

		if (cur->tick < oldest->tick)
			oldest = cur;

		cur = cur->next;
	}

	if (oldest->hole) die("Tried to drop a hole");

	// Is the buffer on either side a hole? If so, merge
	if (oldest->prev && oldest->next && oldest->prev->hole && oldest->next->hole) {
		// Merge two holes
		struct buf *hole1 = oldest->prev;
		struct buf *hole2 = oldest->next;

		hole1->size += oldest->size + hole2->size;
		hole1->next = hole2->next;

		free(hole2);

	} else if (oldest->prev && oldest->prev->hole) {
		struct buf *hole = oldest->prev;
		hole->size += oldest->size;
		hole->next = oldest->next;

	} else if (oldest->next && oldest->next->hole) {
		struct buf *hole = oldest->next;
		hole->size += oldest->size;
		hole->prev = oldest->prev;
	}

	// Drop oldest
	oldest->prev = NULL;
	oldest->next = ctx.ram;
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

	if (!found) die("Asked to find a buffer not in RAM?\n");

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

	// Do we add it to its start or end?
	if (!ctx.edge || mine->size < ctx.edge) {
		// start
		mine->prev = fit->prev;
		mine->next = fit;
		fit->prev = mine;
	} else {
		// end
		mine->prev = fit;
		mine->next = fit->next;
		fit->next = mine;
	}
}

void allocbuf(const u32 id, const u32 size) {

	ctx.tick++;

	struct buf *cur = xcalloc(sizeof(struct buf));
	cur->size = size;
	cur->id = id;
	cur->tick = ctx.tick;

	// Allocate a new buffer, put it to RAM, internaltouch moves it to vram
	cur->next = ctx.ram;
	ctx.ram->prev = cur;
	ctx.ram = cur;

	internaltouch(id);
}

void touchbuf(const u32 id) {

	ctx.tick++;

	struct buf *cur;

	// Is the buffer in VRAM? If so, update its timestamp and quit
	cur = ctx.vram;
	while (cur) {
		if (cur->hole) {
			cur = cur->next;
			continue;
		}

		if (cur->id == id) {
			cur->tick = ctx.tick;
			return;
		}

		cur = cur->next;
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
		die("VRAM got corrupted, size doesn't match\n");
}
