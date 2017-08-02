PREFIX=/usr

MAKE=make
FLEX=flex
BISON=bison
DOXYGEN=doxygen
CPPCHECK=cppcheck
CTAGS=ctags

PYTHON_CFLAGS=-DWITH_PYTHON
PYTHON_LIBS=-lpython2.7 -lffi
PYTHON_LDFLAGS=
PYTHON_INC=-I /usr/include/python2.7

CC=gcc
CFLAGS=-Wall -Wextra -Wno-unused-parameter -std=gnu99 -O2 -D_LARGEFILE64_SOURCE $(PYTHON_CFLAGS) 
LDFLAGS=-L./datatypes $(PYTHON_LDFLAGS)
LIBS=-ldl ./datatypes/libdatatypes-0.2.0.a $(PYTHON_LIBS) -lm
INC=-I./datatypes $(PYTHON_INC)

VERSION=0.2.1

all:
	$(MAKE) -C ./datatypes
	$(FLEX) lexer.l
	$(BISON) parser.y
	$(CC) $(CFLAGS) $(INC) ./main.c ./log.c ./parser.y.c ./lexer.l.c ./format-lexer.c ./format-parser.c ./format.c ./utils.c ./fs.c ./fileinfo.c ./filelist.c ./linux.c ./ast.c ./translate.c ./eval.c ./search.c ./extension.c ./dl-ext-backend.c ./py-ext-backend.c ./blacklist.c -o ./efind $(LDFLAGS) $(LIBS)

install:
	test -d "$(DESTDIR)$(PREFIX)/bin" || mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp ./efind $(DESTDIR)$(PREFIX)/bin/
	chmod 555 $(DESTDIR)$(PREFIX)/bin/efind
	test -d "$(DESTDIR)$(PREFIX)/share/man/man1" || mkdir -p "$(DESTDIR)$(PREFIX)/share/man/man1"
	cp ./man/efind.1 $(DESTDIR)$(PREFIX)/share/man/man1/efind.1
	gzip $(DESTDIR)$(PREFIX)/share/man/man1/efind.1 -f

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/efind
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/efind.1.gz

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

clean:
	$(MAKE) -C ./datatypes clean
	rm -f ./*.o
	rm -f ./lexer.l.h ./lexer.l.c ./parser.y.h ./parser.y.c ./efind
	rm -fr ./doc
	rm -f ./tags
