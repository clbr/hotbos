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
#include <zlib.h>
#include "macros.h"

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

static inline void sread(void *ptr, size_t size, FILE *f) {

	size_t ret = 0;
	while (size) {
		ret = fread(ptr, 1, size, f);
		size -= ret;

		if (!ret) die("Failed reading\n");
	}
}

static inline void sgzread(void *ptr, size_t size, void *f) {

	size_t ret = 0;
	while (size) {
		ret = gzread(f, ptr, size);
		size -= ret;

		if (!ret) die("Failed reading\n");
	}
}

static inline void swrite(const void *ptr, size_t size, FILE *f) {

	size_t ret = 0;
	while (size) {
		ret = fwrite(ptr, 1, size, f);
		size -= ret;

		if (!ret) die("Failed writing\n");
	}
}

static inline CONST_FUNC u8 hex2u8(const char in) {

	if (in <= '9')
		return in - '0';

	return in - 'a' + 10;
}

#endif
