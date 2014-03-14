CFLAGS += -Wall -Wextra -g -ffast-math
CXXFLAGS += -Wall -Wextra -g -fno-exceptions -fno-rtti
LDFLAGS += -Wl,-O1
LDFLAGS += -lz
LDFLAGS += $(CFLAGS)

TARGETS = raw2bin statinfo trainer fragbench fraggraph activation

.PHONY: all clean

all: $(TARGETS)

-include $(wildcard *.d)

raw2bin: raw2bin.o
statinfo: bin.o statinfo.o

trainer: LDFLAGS += -lm
trainer: trainer.o vram.o scoring.o header.o neural.o bin.o bucket.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDCFLAGS) $(LDFLAGS)
fragbench: fragbench.o bin.o fragvram.o

activation: activation.o
activation: LDFLAGS += -lm

fraggraph: CXXFLAGS += $(shell fltk-config --cxxflags)
fraggraph: LDFLAGS += $(shell fltk-config --ldflags)
fraggraph: fraggraph.o
	g++ -o $@ $(CXXFLAGS) $^ $(LDFLAGS)

clean:
	rm -f *.o $(TARGETS)

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<
	$(CC) -MM -MP $(CFLAGS) $(CPPFLAGS)  $*.c -o $*.d > /dev/null &2>&1
