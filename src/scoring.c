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

u32 score(const enum scoredev_t dev, const enum scoreop_t op,
		const enum scoredest_t dest, const u32 size) {

	switch (op) {
		case SCORE_R:
		break;
		case SCORE_W:
		break;
		case SCORE_MOVE:
		break;
	}
}
