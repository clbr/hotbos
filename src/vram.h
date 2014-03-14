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

#ifndef VRAM_H
#define VRAM_H

#include "lrtypes.h"
#include "neural.h"

static const u64 vramsizes[] = { 64, 128, 256, 384, 512, 1024, 1536, 2048, 4096 };
static const u32 vramelements = sizeof(vramsizes) / sizeof(u64);

struct vramctx;

struct vramctx *initvram(const u64 size, const u32 edge, const u32 buffers,
		const struct network *net);
void allocbuf(struct vramctx *, const u32 id, const u32 size, const u8 highprio);
void touchbuf(struct vramctx *, const u32 id, const u8 write);
void destroybuf(struct vramctx *, const u32 id);
u64 freevram(struct vramctx *);
void cpubuf(struct vramctx *, const u32 id);

void checkfragmentation(const struct vramctx *);

#endif
