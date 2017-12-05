PREFIX=/usr
LOCALEDIR=$(PREFIX)/share/locale
LIBDIR=$(PREFIX)/lib

MACHINE:=$(shell uname -m)

ifeq ($(MACHINE), x86_64)
LIBDIR=$(PREFIX)/lib64
endif

MAKE=make
FLEX=flex
BISON=bison
DOXYGEN=doxygen
CPPCHECK=cppcheck
CTAGS=ctags

LIBFFI_CFLAGS=`pkg-config --cflags libffi`
LIBFFI_LDFLAGS=`pkg-config --libs libffi`

PYTHON_CFLAGS=-DWITH_PYTHON -I/usr/include/python2.7 $(LIBFFI_CFLAGS)
PYTHON_LDFLAGS=-lpython2.7 $(LIBFFI_LDFLAGS)

CC=gcc
CFLAGS=-Wall -Wextra -Wno-unused-parameter -std=gnu99 -O2 -D_LARGEFILE64_SOURCE $(PYTHON_CFLAGS) -DLIBDIR=\"$(LIBDIR)\"
LDFLAGS=-L./datatypes $(PYTHON_LDFLAGS) -ldl ./datatypes/libdatatypes-0.2.0.a -lm
INC=-I./datatypes

VERSION=0.3.2

all:
	$(MAKE) -C ./datatypes
	$(FLEX) lexer.l
	$(BISON) parser.y
	$(CC) -DLOCALEDIR=\"$(LOCALEDIR)\" $(CFLAGS) $(INC) ./main.c ./gettext.c ./log.c ./parser.y.c ./lexer.l.c ./format-fields.c ./format-lexer.c ./format-parser.c ./format.c ./utils.c ./fs.c ./fileinfo.c ./filelist.c ./linux.c ./ast.c ./translate.c ./eval.c ./search.c ./extension.c ./dl-ext-backend.c ./py-ext-backend.c ./blacklist.c ./pathbuilder.c -o ./efind $(LDFLAGS) $(LIBS)
	$(MAKE) -C ./po

install:
	test -d "$(DESTDIR)$(PREFIX)/bin" || mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp ./efind $(DESTDIR)$(PREFIX)/bin/
	chmod 555 $(DESTDIR)$(PREFIX)/bin/efind
	test -d "$(DESTDIR)$(PREFIX)/share/man/man1" || mkdir -p "$(DESTDIR)$(PREFIX)/share/man/man1"
	cp ./man/efind.1 $(DESTDIR)$(PREFIX)/share/man/man1/efind.1
	gzip $(DESTDIR)$(PREFIX)/share/man/man1/efind.1 -f
	LOCALEDIR=$(LOCALEDIR) $(MAKE) -C ./po install

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/efind
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/efind.1.gz
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
	$(CPPCHECK) --enable=all --std=c99 --force -j2 --template gcc *.h *.c

pot:
	xgettext --language=C --keyword=_ -o po/efind.pot *.c *.y

clean:
	$(MAKE) -C ./datatypes clean
	rm -f ./*.o
	rm -f ./lexer.l.h ./lexer.l.c ./parser.y.h ./parser.y.c ./efind
	rm -fr ./doc
	rm -f ./tags
	$(MAKE) -C ./po clean
