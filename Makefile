PREFIX=/usr/local

MAKE=make
FLEX=flex
BISON=bison
DOXYGEN=doxygen
CPPCHECK=cppcheck
CTAGS=ctags

CC=gcc
CFLAGS=-Wall -std=gnu99 -O0 -g
LIBS=-ldl -L./datatypes -static -ldatatypes-0.1.0
INC=-I./datatypes

all:
	$(MAKE) -C ./datatypes
	$(FLEX) lexer.l
	$(BISON) parser.y
	$(CC) $(INC) ./main.c ./parser.y.c ./lexer.l.c ./utils.c ./ast.c ./translate.c ./eval.c ./search.c ./extension.c ./dl-ext-backend.c -o ./efind $(CFLAGS) $(LIBS)

install:
	cp ./efind $(PREFIX)/bin/
	chmod 555 $(PREFIX)/bin/efind

uninstall:
	rm -f $(PREFIX)/bin/efind

doc:
	$(DOXYGEN) ./doxygen_config

tags:
	$(CTAGS) -R .

cppcheck:
	$(CPPCHECK) --enable=style --enable=performance --enable=information --std=c99 --force -j2 --template gcc *.h *.c

clean:
	$(MAKE) -C ./datatypes clean
	rm -f ./*.o
	rm -f ./lexer.l.h ./lexer.l.c ./parser.y.h ./parser.y.c ./efind
	rm -fr ./doc
	rm -f ./tags
