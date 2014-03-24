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

#ifndef BUCKET_H
#define BUCKET_H

#include "lrtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bucket;

struct bucket *initbuckets(const u32 bufs);
void freebuckets(struct bucket *b);

void addbucket(struct bucket *b, const u32 id, const u32 score);
void delbucket(struct bucket *b, const u32 id);
void updatebucket(struct bucket *b, const u32 id, const u32 score);

u32 getlowestbucket(const struct bucket *b);

#ifdef __cplusplus
} // extern C
#endif

#endif
