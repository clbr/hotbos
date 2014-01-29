#include <sys/mman.h>
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char **argv) {

	if (argc < 2)
		return 0;

	struct stat st;
	stat(argv[1], &st);

	int fd = open(argv[1], O_RDONLY);

	void *rptr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
	if (rptr == MAP_FAILED)
		return 1;

	char *buf = calloc(1024 * 1024 * 1024, 1);

	z_stream strm;
	memset(&strm, 0, sizeof(z_stream));
	strm.total_in = strm.avail_in = st.st_size;
	strm.total_out = strm.avail_out = 1024*1024*1024;
	strm.next_in = rptr;
	strm.next_out = (void *) buf;

	int ret = inflateInit2(&strm, 15 + 16);
	if (ret != Z_OK)
		return 2;

	ret = inflate(&strm, Z_FINISH);
	if (ret != Z_STREAM_END)
		return 3;
	inflateEnd(&strm);


	munmap(rptr, st.st_size);
	close(fd);
	puts("Success");
	return 0;
}
