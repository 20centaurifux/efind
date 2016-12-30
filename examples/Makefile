all:
	gcc -shared -I../ ./taglib.c -Wall -std=c99 -nostartfiles -fPIC -ltag_c -o ./taglib.so
	gcc -I../ -shared ./gdk-pixbuf.c -Wall -std=c99 -nostartfiles -fPIC `pkg-config --cflags --libs gdk-pixbuf-2.0` -o ./gdk-pixbuf.so

clean:
	rm -f ./taglib.so
	rm -f ./gdk-pixbuf.so
