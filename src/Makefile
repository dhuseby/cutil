# build cutil library
#
SHELL=/bin/sh
SRC=array.c hashtable.c
HDR=array.h debug.h hashtable.h macros.h
OBJ=$(SRC:.c=.o)
OUT=libcutil.a
CFLAGS=-O3 -gstabs+
HEADERDIR=../include
LIBDIR=../lib

all: $(OUT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)
	mkdir -p $(HEADERDIR)/cutil
	mkdir -p $(LIBDIR)
	cp *.h $(HEADERDIR)/cutil/
	cp *.a $(LIBDIR)/

install: all

test: install all

clean:
	rm -rf $(HEADERDIR)
	rm -rf $(LIBDIR)
	rm -rf $(OBJ)
	rm -rf $(OUT)

.PHONY: all install clean test

