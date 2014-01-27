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

#ifndef SCORING_H
#define SCORING_H

#include "lrtypes.h"
#include "macros.h"

enum {
	SCORE_PCIE_LATENCY = 4640000, // https://devtalk.nvidia.com/default/topic/485590/pci-express-latency-and-how-to-decrease-it/
	SCORE_RAM_MULTIPLIER = 18, // 232 / 12.8 GB/s scale difference
	SCORE_STALL_LATENCY = SCORE_PCIE_LATENCY / 2,
	SCORE_CACHED_MULTIPLIER = 3,
};

enum scoredev_t {
	SCORE_CPU,
	SCORE_GPU
};

enum scoredest_t {
	SCORE_RAM,
	SCORE_VRAM
};

enum scoreop_t {
	SCORE_R,
	SCORE_W,
	SCORE_MOVE
};

u32 score(const enum scoredev_t dev, const enum scoreop_t op,
		const enum scoredest_t dest, const u64 size) CONST_FUNC;

#endif
