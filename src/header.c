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
#include "neural.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define HDR "magic.h"

void readhdr(struct network * const in, const char name[]) {

	FILE * const f = fopen(name, "r");
	if (!f) die("Failed to open file %s\n", name);

	enum {
		bufsize = 160
	};

	char buf[bufsize];
	while (fgets(buf, bufsize, f)) {
		nukenewline(buf);

		if (!strchr(buf, '#'))
			continue;
		if (strstr(buf, "_H") || !strstr(buf, "define"))
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

	fputs("/*\n"
		" * Buffer scoring AI, autogenerated scores\n"
		" *\n"
		" * Copyright (C) 2014 Lauri Kasanen   All Rights Reserved.\n"
		" *\n"
		" * Permission is hereby granted, free of charge, to any person obtaining a\n"
		" * copy of this software and associated documentation files (the \"Software\"),\n"
		" * to deal in the Software without restriction, including without limitation\n"
		" * the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
		" * and/or sell copies of the Software, and to permit persons to whom the\n"
		" * Software is furnished to do so, subject to the following conditions:\n"
		" *\n"
		" * The above copyright notice and this permission notice shall be included\n"
		" * in all copies or substantial portions of the Software.\n"
		" *\n"
		" * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS\n"
		" * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		" * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL\n"
		" * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR\n"
		" * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,\n"
		" * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
		" * OTHER DEALINGS IN THE SOFTWARE.\n"
		" */\n\n", f);

	fputs("#ifndef MAGIC_H\n#define MAGIC_H\n\n", f);

	u32 i, j;
	for (i = 0; i < INPUT_NEURONS; i++) {
		fprintf(f, "#define input%uw %f\n",
			i, in->input[i].weight);
		fprintf(f, "#define input%ub %f\n",
			i, in->input[i].bias);
	}

	for (i = 0; i < HIDDEN_NEURONS; i++) {
		for (j = 0; j < INPUT_NEURONS; j++) {
			fprintf(f, "#define hidden%uw%u %f\n",
				i, j, in->hidden[i].weights[j]);
		}
		fprintf(f, "#define hidden%ub %f\n",
			i, in->hidden[i].bias);
	}

	for (i = 0; i < HIDDEN_NEURONS; i++) {
		fprintf(f, "#define outputw%u %f\n",
			i, in->output.weights[i]);
	}
	fprintf(f, "#define outputb %f\n",
		in->output.bias);

	fputs("\n#endif\n", f);
	fclose(f);
}
