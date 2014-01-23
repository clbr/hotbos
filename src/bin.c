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

#include "lrtypes.h"
#include "bin.h"
#include "helpers.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

void *gzbinopen(const char in[], u32 * const size) {

	void * const f = gzopen(in, "rb");
	if (!f) die("Failed to open file\n");

	struct stat st;
	if (stat(in, &st))
		die("Failed in stat\n");
	*size = st.st_size;

	char magic[MAGICLEN];
	sgzread(magic, MAGICLEN, f);
	if (memcmp(magic, MAGIC, MAGICLEN))
		die("This is not a bostats binary file.\n");

	return f;
}
