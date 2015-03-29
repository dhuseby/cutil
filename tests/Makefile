# Copyright (c) 2012-2015 David Huseby
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#
# build cutil test executable
#

# definable variables
EXTRA_LIBS_ROOT?=/usr/local

# other variables
SHELL=/bin/sh
#SRC=test_all.c test_aiofd.c test_bitset.c test_btree.c test_buffer.c test_cb.c test_child.c test_events.c test_flags.c test_hashtable.c test_list.c test_pair.c test_privileges.c test_sanitize.c test_socket.c
SRC=test_all.c test_aiofd.c test_cb.c test_events.c test_flags.c test_hashtable.c test_list.c test_pair.c test_socket.c
OBJ=$(SRC:.c=.o)
GCDA=$(SRC:.c=.gcda)
GCNO=$(SRC:.c=.gcno)
GCOV=$(SRC:.c=.c.gcov)
OUT=test_all
LIBS=-lcutil -lcunit -lev -lm -lpthread
CUTIL_ROOT=../src
CFLAGS=-O0 -gstabs+ -DUNIT_TESTING -I$(CUTIL_ROOT)/include -I$(EXTRA_LIBS_ROOT)/include
LDFLAGS=-O0 -gstabs+ -L$(CUTIL_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib
# set LDFLAGS to something gcov can use
coverage: LDFLAGS=-fprofile-arcs -O0 -gstabs+ -L$(CUTIL_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib


all:

test: $(OUT)
	./test_all

# build test_all but don't run it
testnr: $(OUT)

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

