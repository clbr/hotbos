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

#include "scoring.h"
#include "lrtypes.h"
#include "helpers.h"

u32 score(const enum scoredev_t dev, const enum scoreop_t op,
		const enum scoredest_t dest, const u32 size) {

	u32 out = 0;

	if (op == SCORE_MOVE) {
		// The higher rw score + stall + pcie latency
		out = size * SCORE_RAM_MULTIPLIER + SCORE_STALL_LATENCY +
			SCORE_PCIE_LATENCY;
		return out;
	}

	if (dev == SCORE_CPU) {
		switch (op) {
			case SCORE_R:
			case SCORE_W:
				if (dest == SCORE_RAM)
					out = size * SCORE_RAM_MULTIPLIER;
				else if (dest == SCORE_VRAM)
					out = size + SCORE_PCIE_LATENCY;
				else
					die("Unhandled score dest\n");
			break;
			default:
				die("Unhandled score op\n");
		}
	} else if (dev == SCORE_GPU) {
		switch (op) {
			case SCORE_R:
				if (dest == SCORE_RAM)
					out = size * SCORE_RAM_MULTIPLIER +
						SCORE_PCIE_LATENCY;
				else if (dest == SCORE_VRAM)
					out = size;
				else
					die("Unhandled score dest\n");
			break;
			case SCORE_W:
				if (dest == SCORE_RAM)
					out = size * SCORE_RAM_MULTIPLIER *
						SCORE_CACHED_MULTIPLIER +
						SCORE_PCIE_LATENCY;
				else if (dest == SCORE_VRAM)
					out = size;
				else
					die("Unhandled score dest\n");
			break;
			default:
				die("Unhandled score op\n");
		}
	} else {
		die("Unknown score device\n");
	}

	return out;
}
