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
#include <map>

using namespace std;

struct bucket {
	u64 *scores;
	multimap<u64, u32> *queue;
};

typedef pair<const u64, u32> mypair;
typedef multimap<u64, u32>::iterator myiter;

struct bucket *initbuckets(const u32 bufs) {

	struct bucket *b = (struct bucket *) xcalloc(sizeof(struct bucket));
	b->scores = (u64 *) xcalloc(sizeof(u64) * bufs);
	b->queue = new multimap<u64, u32>();

	return b;
}

void freebuckets(struct bucket * const b) {

	delete b->queue;
	free(b->scores);
	free(b);
}

void addbucket(struct bucket * const b, const u32 id, const u64 score) {

	b->scores[id] = score;
	b->queue->insert(mypair(score, id));
}

void delbucket(struct bucket * const b, const u32 id) {

	const u64 oldscore = b->scores[id];

	b->scores[id] = 0;

	pair<myiter, myiter> range = b->queue->equal_range(oldscore);
	for (myiter it = range.first; it != range.second; it++) {
		if (it->second == id) {
			b->queue->erase(it);
			break;
		}
	}
}

void updatebucket(struct bucket * const b, const u32 id,
			const u64 score) {

	delbucket(b, id);
	addbucket(b, id, score);
}

u32 getlowestbucket(const struct bucket * const b) {

	const myiter it = b->queue->begin();
	return it->second;
}
