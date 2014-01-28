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

#include "header.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define HDR "magic.h"

void readhdr(struct network * const in) {

	FILE * const f = fopen(HDR, "r");
	if (!f) die("Failed to open file\n");

	enum {
		bufsize = 160
	};

	char buf[bufsize];
	while (fgets(buf, bufsize, f)) {
		nukenewline(buf);

		if (!strchr(buf, '#'))
			continue;

		char *param = strchr(buf, ' ');
		if (!param) die("Corrupted header\n");
		while (isspace(*param)) param++;

		char *arg = strchr(param, ' ');
		if (!arg) die("Corrupted header\n");
		while (isspace(*arg)) arg++;

		#define ARG(a) if (!strncmp(a, param, sizeof(a) - 1))

		u8 neuron;
		u8 weight;

		ARG("input") {
			param += 5;
			neuron = *param - '0';
			param++;

			if (*param == 'w') {
				in->input[neuron].weight = strtof(arg, NULL);
			} else if (*param == 'b') {
				in->input[neuron].bias = strtof(arg, NULL);
			} else die("Unknown type\n");

		} else ARG("hidden") {
			param += 6;
			neuron = *param - '0';
			param++;

			if (*param == 'w') {
				param++;
				weight = *param - '0';
				in->hidden[neuron].weights[weight] = strtof(arg, NULL);
			} else if (*param == 'b') {
				in->hidden[neuron].bias = strtof(arg, NULL);
			} else die("Unknown type\n");

		} else ARG("output") {
			param += 6;

			if (*param == 'w') {
				param++;
				weight = *param - '0';
				in->output.weights[weight] = strtof(arg, NULL);
			} else if (*param == 'b') {
				in->output.bias = strtof(arg, NULL);
			} else die("Unknown type\n");
		}

		#undef ARG
	}

	fclose(f);
}

void writehdr(const struct network * const in) {

	FILE * const f = fopen(HDR, "w");
	if (!f) die("Failed to open file\n");
	fclose(f);

}
