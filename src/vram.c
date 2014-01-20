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
	u32 id;
	u32 size;
	struct buf *next;
};

static struct {
	u32 size;
	u32 edge;

	struct buf *vram;
	struct buf *ram;
} ctx;

void initvram(const u32 size, const u32 edge) {

	ctx.size = size;
	ctx.edge = edge;
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

void allocbuf(const u32 id, const u32 size) {

}

void touchbuf(const u32 id) {

}

void checkfragmentation() {

}
