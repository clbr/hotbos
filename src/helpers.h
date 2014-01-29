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

#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <zlib.h>
#include "macros.h"
#include "lrtypes.h"

static inline NORETURN_FUNC PRINTF_FUNC(1, 2) void die(const char fmt[], ...) {

	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(1);
}

static inline MALLOC_FUNC void *xcalloc(unsigned bytes) {

	void *ptr = calloc(1, bytes);
	if (!ptr) die("OOM\n");

	return ptr;
}

static inline void sread(char *ptr, size_t size, FILE *f) {

	size_t ret = 0;
	u32 pos = 0;
	while (size) {
		ret = fread(ptr + pos, 1, size, f);
		size -= ret;
		pos += ret;

		if (!ret) die("Failed reading\n");
	}
}

static inline void sgzread(char *ptr, size_t size, void *f) {

	size_t ret = 0;
	u32 pos = 0;
	while (size) {
		ret = gzread(f, ptr + pos, size);
		size -= ret;
		pos += ret;

		if (ret <= 0) die("Failed reading\n");
	}
}

static inline void swrite(const char *ptr, size_t size, FILE *f) {

	size_t ret = 0;
	u32 pos = 0;
	while (size) {
		ret = fwrite(ptr + pos, 1, size, f);
		size -= ret;
		pos += ret;

		if (!ret) die("Failed writing\n");
	}
}

static inline CONST_FUNC u8 hex2u8(const char in) {

	if (in <= '9')
		return in - '0';

	return in - 'a' + 10;
}

static inline void nukenewline(char buf[]) {

	char *ptr = strchr(buf, '\n');
	if (ptr)
		*ptr = '\0';
}

static inline int filterdata(const struct dirent * const d) {

	if (d->d_type != DT_REG && d->d_type != DT_UNKNOWN)
		return 0;

	if (strstr(d->d_name, ".bin"))
		return 1;

	return 0;
}

#endif
