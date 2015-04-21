FLEX=flex
BISON=bison
DOXYGEN=doxygen

CC=gcc
CFLAGS=-Wall -std=gnu99

all:
	$(FLEX) lexer.l
	$(BISON) parser.y
	$(CC) main.c parser.y.c lexer.l.c buffer.c utils.c ast.c translate.c search.c -o efind $(CFLAGS)

doc:
	$(DOXYGEN) ./doxygen_config

clean:
	rm -f ./*.o
	rm -f ./lexer.l.h ./lexer.l.c ./parser.y.h ./parser.y.c ./efind
	rm -fr ./doc
