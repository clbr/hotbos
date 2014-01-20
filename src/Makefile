CFLAGS += -Wall -Wextra -g
LDFLAGS += -Wl,-O1
LDFLAGS += -lz
LDFLAGS += $(CFLAGS)

TARGETS = raw2bin statinfo trainer fragbench

.PHONY: all clean

all: $(TARGETS)

-include $(wildcard *.d)

raw2bin: raw2bin.o
statinfo: bin.o statinfo.o
trainer: trainer.o
fragbench: fragbench.o bin.o

clean:
	rm -f *.o $(TARGETS)

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<
	$(CC) -MM -MP $(CFLAGS) $(CPPFLAGS)  $*.c -o $*.d > /dev/null &2>&1
