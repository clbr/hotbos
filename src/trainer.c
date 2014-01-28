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

#define _GNU_SOURCE

#include "bin.h"
#include "vram.h"
#include "helpers.h"
#include <unistd.h>
#include <getopt.h>

static void usage(const char name[]) {
	die("Usage: %s\n\n"
		"Modes:\n"
		"	-b --benchmark	Test the current state vs LRU (default)\n"
		"	-r --revolve	Revolutions\n"
		"	-e --evolve	Evolutions\n"
		"	-f --finetune	Fine tuning\n\n"
		"Options:\n"
		"	-v --vram 64	Only test this vram amount\n"
		, name);
}

int main(int argc, char **argv) {

	usage(argv[0]);

	return 0;
}
