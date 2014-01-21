#include <stdio.h>
#include <string.h>
#include <lrtypes.h>

int main() {

	enum { bufsize = 2 };
	char buf[bufsize];

	while (1) {
		size_t ret = fread(buf, 1, bufsize, stdin);
		if (!ret) break;
		if (ret == 1) {
			putchar(buf[0]);
			break;
		}

		// Got two bytes.
		check:
		if (buf[0] == '\n' && buf[1] == 's') {
			buf[0] = 's';
			putchar(buf[0]);
			putchar(buf[1]);
			continue;
		}

		if (buf[1] == '\n') {
			putchar(buf[0]);
			buf[0] = buf[1];
			ret = fread(buf + 1, 1, 1, stdin);
			if (!ret) break;

			goto check;
		}

		putchar(buf[0]);
		putchar(buf[1]);
	}

	return 0;
}
