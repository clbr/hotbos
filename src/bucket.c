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

#include "bucket.h"
#include "helpers.h"
#include <limits.h>

enum {
	BUCKETS = 65536,
};

struct node {
	u32 score;
	struct node *next;
	struct node *prev;
};

struct bucket {
	struct node *bucket[BUCKETS];
	struct node *nodes;

	u32 lowest;
	u32 entries;
};

static void updatelowest(struct bucket * const b) {

	if (!b->entries) {
		b->lowest = 0;
		return;
	}

	u32 i;
	for (i = 0; i < BUCKETS; i++) {
		if (b->bucket[i]) {
			b->lowest = i;
			break;
		}
	}
}

struct bucket *initbuckets(const u32 bufs) {

	struct bucket *b = xcalloc(sizeof(struct bucket));
	b->nodes = xcalloc(sizeof(struct node) * bufs);

	return b;
}

void freebuckets(struct bucket * const b) {

	free(b->nodes);
	free(b);
}

void addbucket(struct bucket * const b, const u32 id, const u32 score) {

	b->entries++;

	const u32 hash = score / BUCKETS;
	b->nodes[id].score = score;

	b->nodes[id].next = b->bucket[hash];
	if (b->bucket[hash])
		b->bucket[hash]->prev = &b->nodes[id];
	b->bucket[hash] = &b->nodes[id];

	updatelowest(b);
}

void delbucket(struct bucket * const b, const u32 id) {

	b->entries--;

	// Find it
	struct node *cur = &b->nodes[id];
	const u32 hash = cur->score / BUCKETS;

	// Is it the first?
	if (cur == b->bucket[hash]) {
		b->bucket[hash] = cur->next;
		if (cur->next)
			b->bucket[hash]->prev = NULL;
	} else {
		struct node * const prev = cur->prev;
		struct node * const next = cur->next;

		if (prev)
			prev->next = next;
		if (next)
			next->prev = prev;
	}

	cur->score = 0;
	cur->prev = cur->next = NULL;

	updatelowest(b);
}

void updatebucket(struct bucket * const b, const u32 id,
			const u32 score) {

	delbucket(b, id);
	addbucket(b, id, score);
}

u32 getlowestbucket(const struct bucket * const b) {

	struct node *cur = b->bucket[b->lowest];
	struct node *target = cur;
	if (!cur || !b->entries) die("Tried to get NULL lowest\n");
	cur = cur->next;

	while (cur) {
		if (cur->score <= target->score)
			target = cur;

		cur = cur->next;
	}

	return (target - b->nodes);
}
