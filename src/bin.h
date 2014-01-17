#ifndef BIN_H
#define BIN_H

#include "lrtypes.h"

/* One entry takes five bytes, as follows:

	struct {
		u8 id: 2;
		u32 time: 22;
		u16 buffer;
	}

   Create entries are followed by four bytes:

	struct {
		u8 high_prio: 1;
		u32 size: 31;
	}
*/

#endif
