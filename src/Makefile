CXXFLAGS += -Wall -Wextra
LDFLAGS += -Wl,-O1

TARGETS = raw2bin

.PHONY: all clean

all: $(TARGETS)
$(TARGETS): $(wildcard *.h)

raw2bin: raw2bin.o

clean:
	rm -f *.o $(TARGETS)
