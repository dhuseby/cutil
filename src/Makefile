# build cutil library
#
SHELL=/bin/sh
NAME=cutil
SRC=array.c bitset.c hashtable.c
HDR=array.h bitset.h debug.h hashtable.h macros.h
OBJ=$(SRC:.c=.o)
OUT=lib$(NAME).a
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
	rm -rf $(NAME)
	rm -rf include
	rm -rf lib
	ln -s . $(NAME)
	ln -s . include
	ln -s . lib

install: all
	mkdir -p $(PREFIX)/include/$(NAME)
	mkdir -p $(PREFIX)/lib
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/include/$(NAME)/ *.h
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/lib/ *.a

uninstall:
	rm -rf $(PREFIX)/include/$(NAME)
	rm -rf $(PREFIX)/lib/$(OUT)

test: all

clean:
	rm -rf $(NAME)
	rm -rf include
	rm -rf lib
	rm -rf $(OBJ)
	rm -rf $(OUT)

.PHONY: all install uninstall clean test

