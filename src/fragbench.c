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
#include <unistd.h>
#include <string.h>

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

	return 0;
}
