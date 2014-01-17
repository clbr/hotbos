#ifndef BIN_H
#define BIN_H

#include "lrtypes.h"

/* One entry takes five bytes, as follows:

	struct {
		u8 id: 3;
		u32 time: 21;
		u16 buffer;
	}

   Create entries are followed by four bytes:

	struct {
		u8 high_prio: 1;
		u32 size: 31;
	}
*/

enum id_t {
	ID_CREATE = 0,
	ID_READ = 1,
	ID_WRITE = 2,
	ID_DESTROY = 3,
	ID_CPUOP = 4
};

typedef struct {
	u32 time;
	u32 size;
	enum id_t id;
	u16 buffer;
	u8 high_prio;
} entry;

#endif
