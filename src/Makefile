CFLAGS += -Wall -Wextra
LDFLAGS += -Wl,-O1

TARGETS = raw2bin statinfo trainer

.PHONY: all clean

all: $(TARGETS)

raw2bin: raw2bin.o
statinfo: bin.o statinfo.o
trainer: trainer.o

clean:
	rm -f *.o $(TARGETS)
