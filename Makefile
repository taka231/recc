CFLAGS=-std=c11 -g -static

recc: recc.c

test: recc
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean
