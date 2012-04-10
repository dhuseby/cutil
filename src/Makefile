# build cutil library
#
SHELL=/bin/sh
SRC=array.c hashtable.c
HDR=array.h debug.h hashtable.h macros.h
OBJ=$(SRC:.c=.o)
OUT=libcutil.a
CFLAGS=-O3 -gstabs+
PREFIX=/usr/local

# install utility
INSTALL=/usr/bin/install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644

all: $(OUT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)
	ln -s . cutil

install: all
	mkdir -p $(PREFIX)/include/cutil
	mkdir -p $(PREFIX)/lib
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/include/cutil/ *.h
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/lib/ *.a

uninstall:
	rm -rf $(PREFIX)/include/cutil
	rm -rf $(PREFIX)/lib/$(OUT)

test: all
	mkdir -p ../include/cutil
	mkdir -p ../lib/
	cp *.h ../include/cutil
	cp *.a ../lib/

clean:
	rm -rf ../include
	rm -rf ../lib
	rm -rf cutil
	rm -rf $(OBJ)
	rm -rf $(OUT)

.PHONY: all install clean test

