# build cutil test executable
#
SHELL=/bin/sh
SRC=test_all.c test_array.c test_bitset.c test_btree.c test_buffer.c test_hashtable.c test_pair.c
OBJ=$(SRC:.c=.o)
GCDA=$(SRC:.c=.gcda)
GCNO=$(SRC:.c=.gcno)
GCOV=$(SRC:.c=.c.gcov)
OUT=test_all
LIBS=-lcutil -lcunit
EXTRA_LIBS_ROOT=/usr/local
CUTILS_ROOT=../src
CFLAGS=-O0 -gstabs+ -I$(CUTILS_ROOT)/include -I$(EXTRA_LIBS_ROOT)/include
LDFLAGS=-O0 -gstabs+ -L$(CUTILS_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib
# set LDFLAGS to something gcov can use
coverage: LDFLAGS=-fprofile-arcs -O0 -gstabs+ -L$(CUTILS_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib


all:

test: $(OUT)
	./test_all

coverage: $(OUT)
	./test_all

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

install:

uninstall:

report:

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)
	rm -rf $(GCDA)
	rm -rf $(GCNO)
	rm -rf $(GCOV)

.PHONY: all install uninstall clean test coverage report

