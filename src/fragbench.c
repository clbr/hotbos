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

#include "bin.h"
#include "helpers.h"
#include "vram.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

static int filterdata(const struct dirent * const d) {

	if (d->d_type != DT_REG && d->d_type != DT_UNKNOWN)
		return 0;

	if (strstr(d->d_name, ".bin"))
		return 1;

	return 0;
}

int main(int argc, char **argv) {

	if (argc > 1 && argv[1][0] == '-') {
		die("Usage: %s [optional strategy]\n\n"
			"Strategies: default, minmax (number)\n",
			argv[0]);
	}

	const char *datadir = NULL;

	// Locate the data files
	if (!access("INDEX", R_OK))
		datadir = ".";
	else if (!access("data/INDEX", R_OK))
		datadir = "data";
	else if (!access("../data/INDEX", R_OK))
		datadir = "../data";

	if (!datadir) die("Cannot find data\n");

	// Determine strategy
	u32 amount = 0;
	if (!argv[1] || !strcmp(argv[1], "default")) {
		fprintf(stderr, "Measuring default strategy.\n");
	} else if (argc > 2 && !strcmp(argv[1], "minmax")) {
		amount = atoi(argv[2]);
		fprintf(stderr, "Measuring minmax strategy, with edge %u mb.\n",
			amount);
	} else {
		die("Unknown strategy\n");
	}

	// Loop over the data files
	struct dirent **namelist;
	int datafiles = scandir(datadir, &namelist, filterdata, alphasort);
	if (datafiles < 0) die("Failed in scandir\n");

	u32 i;
	for (i = 0; i < (u32) datafiles; i++) {
	}

	return 0;
}
