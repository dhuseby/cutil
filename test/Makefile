# build cutil test executable
#
SHELL=/bin/sh
SRC=test_all.c test_array.c test_bitset.c test_buffer.c test_hashtable.c test_pair.c
OBJ=$(SRC:.c=.o)
OUT=test_all
LIBS=-lcutil -lcunit
EXTRA_LIBS_ROOT=/usr/local
CUTILS_ROOT=../src
CFLAGS=-O3 -gstabs+ -I$(CUTILS_ROOT)/include -I$(EXTRA_LIBS_ROOT)/include
LDFLAGS=-gstabs+ -L$(CUTILS_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib

all:

test: $(OUT)
	./test_all

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

install:

uninstall:

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)

.PHONY: all install uninstall clean test

