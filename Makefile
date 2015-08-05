PREFIX=/usr/local

FLEX=flex
BISON=bison
DOXYGEN=doxygen

CC=gcc
CFLAGS=-Wall -std=gnu99
LIBS=-lbsd -ldatatypes-0.1
INC=-I/usr/local/include/datatypes

all:
	$(FLEX) lexer.l
	$(BISON) parser.y
	$(CC) $(INC) ./main.c ./parser.y.c ./lexer.l.c ./utils.c ./ast.c ./translate.c ./search.c -o ./efind $(CFLAGS) $(LIBS)

install:
	cp ./efind $(PREFIX)/bin/
	chmod 555 $(PREFIX)/bin/efind

uninstall:
	rm -f $(PREFIX)/bin/efind

doc:
	$(DOXYGEN) ./doxygen_config

clean:
	rm -f ./*.o
	rm -f ./lexer.l.h ./lexer.l.c ./parser.y.h ./parser.y.c ./efind
	rm -fr ./doc
