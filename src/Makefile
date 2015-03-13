# build cutil library
#

# definable variables
PREFIX?=/usr/local
COVERAGE?=../coverage
EXTRA_LIBS_ROOT?=/usr/local

# other variables
SHELL=/bin/sh
NAME=cutil
SRC=aiofd.c bitset.c btree.c buffer.c cb.c child.c daemon.c events.c hashtable.c list.c log.c pair.c privileges.c sanitize.c socket.c
HDR=aiofd.h bitset.h btree.h buffer.h cb.h child.h daemon.h debug.h events.h hashtable.h list.h log.h macros.h pair.h privileges.h sanitize.h socket.h
OBJ=$(SRC:.c=.o)
OUT=lib$(NAME).a
GCDA=$(SRC:.c=.gcda)
GCNO=$(SRC:.c=.gcno)
GCOV=$(SRC:.c=.c.gcov)
# set CFLAGS to something gcov can use
CFLAGS=-O3 -I$(EXTRA_LIBS_ROOT)/include
coverage: CFLAGS=-fprofile-arcs -ftest-coverage -O0 -gstabs+ -DUNIT_TESTING -I$(EXTRA_LIBS_ROOT)/include -I../tests
test: CFLAGS=-O0 -gstabs+ -DUNIT_TESTING -I$(EXTRA_LIBS_ROOT)/include -I../tests
testnr: CFLAGS=-O0 -gstabs+ -DUNIT_TESTING -I$(EXTRA_LIBS_ROOT)/include -I../tests
debug: CFLAGS=-O0 -gstabs+ -I$(EXTRA_LIBS_ROOT)/include

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

testnr: all

coverage: all

debug: all

report:
	lcov --compat split_crc=on --base-directory . --directory . -c -o $(NAME).info
	lcov --compat split_crc=on --remove $(NAME).info "/usr*" -o $(NAME).info
	rm -rf $(COVERAGE)
	genhtml -o $(COVERAGE) -t "$(NAME) test coverage" --num-spaces 4 $(NAME).info

clean:
	rm -rf $(NAME)
	rm -rf include
	rm -rf lib
	rm -rf $(OBJ)
	rm -rf $(OUT)
	rm -rf $(GCDA)
	rm -rf $(GCNO)
	rm -rf $(GCOV)
	rm -rf ev.h.gcov

.PHONY: all install uninstall clean test debug coverage report

