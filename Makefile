PREFIX?=/usr
LOCALEDIR?=$(PREFIX)/share/locale
SYSCONFDIR?=/etc
DATAROOTDIR=$(PREFIX)/share

MACHINE:=$(shell uname -m)

ifeq ($(MACHINE), x86_64)
	LIBDIR?=$(PREFIX)/lib64
else
	LIBDIR?=$(PREFIX)/lib
endif

MAKE?=make
FLEX?=flex
BISON?=bison
DOXYGEN?=doxygen
CPPCHECK?=cppcheck
CTAGS?=ctags
PKG_CONFIG?=pkg-config
PYTHON_CONFIG?=python3-config

LIBFFI_CFLAGS=`$(PKG_CONFIG) --cflags libffi`
LIBFFI_LDFLAGS=`$(PKG_CONFIG) --libs libffi`

PYTHON_CFLAGS?=-DWITH_PYTHON $(shell $(PKG_CONFIG) python3 --cflags) $(LIBFFI_CFLAGS)

ifneq (, $(shell which python3.6 2>/dev/null))
	PYTHON_LDFLAGS?=$(shell $(PYTHON_CONFIG) --libs) $(LIBFFI_LDFLAGS)
else ifneq (, $(shell which python3.7 2>/dev/null))
	PYTHON_LDFLAGS?=$(shell $(PYTHON_CONFIG) --libs) $(LIBFFI_LDFLAGS)
else ifneq (, $(shell which python3.8 2>/dev/null))
	PYTHON_LDFLAGS?=$(shell $(PKG_CONFIG) python3.8 --libs) $(LIBFFI_LDFLAGS)
else ifneq (, $(shell which python3.9 2>/dev/null))
	PYTHON_LDFLAGS?=$(shell $(PYTHON_CONFIG) --ldflags --embed) $(LIBFFI_LDFLAGS)
endif

INIH_CFLAGS?=-DINI_USE_STACK=0 -I"$(PWD)/inih"

CC?=gcc
override CFLAGS+=$(PYTHON_CFLAGS) $(INIH_CFLAGS) -Wall -Wextra -Wno-unused-parameter -std=gnu99 -O2 -D_LARGEFILE64_SOURCE -DLIBDIR=\"$(LIBDIR)\" -DSYSCONFDIR=\"$(SYSCONFDIR)\" 
override LDFLAGS+=-L./datatypes $(PYTHON_LDFLAGS) -ldl ./datatypes/libdatatypes.a.0.3.2 -lm
INC=-I"$(PWD)/datatypes"

VERSION=0.5.6

all:
	$(MAKE) -C ./datatypes
	$(FLEX) lexer.l
	$(BISON) parser.y
	$(CC) -DLOCALEDIR=\"$(LOCALEDIR)\" $(CFLAGS) $(INC) ./main.c ./processor.c ./range.c ./print.c ./exec.c ./sort.c ./gettext.c ./log.c ./options_getopt.c ./options_ini.c ./inih/ini.c ./exec-args.c ./parser.y.c ./lexer.l.c ./format-fields.c ./format-lexer.c ./format-parser.c ./format.c ./utils.c ./fs.c ./fileinfo.c ./filelist.c ./linux.c ./ast.c ./translate.c ./eval.c ./search.c ./extension.c ./dl-ext-backend.c ./py-ext-backend.c ./blacklist.c ./pathbuilder.c -o ./efind $(LDFLAGS) $(LIBS)
	$(MAKE) -C ./po

install:
	test -d "$(DESTDIR)$(PREFIX)/bin" || mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp ./efind $(DESTDIR)$(PREFIX)/bin/
	chmod 555 $(DESTDIR)$(PREFIX)/bin/efind
	test -d "$(DESTDIR)$(PREFIX)/share/man/man1" || mkdir -p "$(DESTDIR)$(PREFIX)/share/man/man1"
	cp ./man/efind.1 $(DESTDIR)$(PREFIX)/share/man/man1/efind.1
	gzip $(DESTDIR)$(PREFIX)/share/man/man1/efind.1 -f
	test -d "$(DESTDIR)$(DATAROOTDIR)/efind" || mkdir -p "$(DESTDIR)$(DATAROOTDIR)/efind"
	cp ./share/* $(DESTDIR)$(DATAROOTDIR)/efind/
	LOCALEDIR=$(LOCALEDIR) $(MAKE) -C ./po install

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/efind
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/efind.1.gz
	rm -fr $(DESTDIR)$(DATAROOTDIR)/efind
	LOCALEDIR=$(LOCALEDIR) $(MAKE) -C ./po uninstall

tarball:
	cd .. && \
	rm -rf ./efind-$(VERSION) && \
	cp -r ./efind ./efind-$(VERSION) && \
	find ./efind-$(VERSION) -name ".git*" | xargs rm -r && \
	tar cfJ ./efind-$(VERSION).tar.xz ./efind-$(VERSION) --remove-files 

doc:
	$(DOXYGEN) ./doxygen_config

tags:
	$(CTAGS) -R .

cppcheck:
	$(CPPCHECK) $(FLAGS) $(INC) --enable=warning --enable=style --enable=performance --enable=portability --enable=information \
	            --suppress=missingIncludeSystem --template gcc *.h *.c

pot:
	xgettext --language=C --keyword=_ -o po/efind.pot *.c *.y

clean:
	$(MAKE) -C ./datatypes clean
	rm -f ./*.o
	rm -f ./lexer.l.h ./lexer.l.c ./parser.y.h ./parser.y.c ./efind
	rm -fr ./doc
	rm -f ./tags
	$(MAKE) -C ./po clean
