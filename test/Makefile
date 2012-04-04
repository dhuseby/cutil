# build cutil test executable
#
SHELL=/bin/sh
SRC=test_all.c test_array.c test_hashtable.c
OBJ=$(SRC:.c=.o)
OUT=test_all
CFLAGS=-O3 -gstabs+ -I../include
LDFLAGS=-gstabs+ -L../lib
LIBS=-lcutil -lcunit

all:

test: $(OUT)
	./test_all

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

install:

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)

.PHONY: all install clean test

