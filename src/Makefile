CFLAGS += -Wall -Wextra
LDFLAGS += -Wl,-O1

TARGETS = raw2bin

.PHONY: all clean

all: $(TARGETS)

raw2bin: raw2bin.o

clean:
	rm -f *.o $(TARGETS)
