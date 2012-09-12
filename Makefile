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
.PHONY: subdirs $(DEBUGDIRS)
.PHONY: subdirs $(GCOVDIRS)
.PHONY: subdirs $(REPORTDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all install uninstall clean test debug coverage report

