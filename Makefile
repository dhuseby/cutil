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
# recursively build cutil
#
SHELL=/bin/sh
INSTALL=/usr/bin/install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644
COVERAGE?=./coverage

SRCDIR = src
TESTDIR = tests
DIRS = $(SRCDIR) $(TESTDIR)
BUILDDIRS = $(DIRS:%=build-%)
INSTALLDIRS = $(DIRS:%=install-%)
UNINSTALLDIRS = $(DIRS:%=uninstall-%)
CLEANDIRS = $(DIRS:%=clean-%)
TESTDIRS = $(DIRS:%=test-%)
TESTNRDIRS = $(DIRS:%=testnr-%)
DEBUGDIRS = $(SRCDIR:%=debug-%)
GCOVDIRS = $(DIRS:%=gcov-%)
REPORTDIRS = $(DIRS:%=report-%)

all: $(BUILDDIRS)

$(DIRS): $(BUILDDIRS)

$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

install: $(INSTALLDIRS)

$(INSTALLDIRS):
	$(MAKE) -C $(@:install-%=%) install

uninstall: $(UNINSTALLDIRS)

$(UNINSTALLDIRS):
	$(MAKE) -C $(@:uninstall-%=%) uninstall

test: $(TESTDIRS)

$(TESTDIRS):
	$(MAKE) -C $(@:test-%=%) test

testnr: $(TESTNRDIRS)

$(TESTNRDIRS):
	$(MAKE) -C $(@:testnr-%=%) testnr

debug: $(DEBUGDIRS)

$(DEBUGDIRS):
	$(MAKE) -C $(@:debug-%=%) debug

coverage: $(GCOVDIRS) $(REPORTDIRS)

$(GCOVDIRS):
	$(MAKE) -C $(@:gcov-%=%) coverage

report: $(REPORTDIRS)

$(REPORTDIRS):
	$(MAKE) -C $(@:report-%=%) report

clean: $(CLEANDIRS)

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

.PHONY: subdirs $(DIRS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(INSTALLDIRS)
.PHONY: subdirs $(UNINSTALL)
.PHONY: subdirs $(TESTDIRS)
.PHONY: subdirs $(TESTNRDIRS)
.PHONY: subdirs $(DEBUGDIRS)
.PHONY: subdirs $(GCOVDIRS)
.PHONY: subdirs $(REPORTDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all install uninstall clean test testnr debug coverage report

