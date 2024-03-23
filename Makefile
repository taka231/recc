CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

recc: $(OBJS)
	$(CC) -o recc $(OBJS) $(LDFLAGS)

$(OBJS): recc.h

test/%.exe: recc test/%.c
	$(CC) -o- -E -P -C test/$*.c > test/tmp_$*.c
	./recc test/tmp_$*.c > test/$*.s
	$(CC) -o $@ test/$*.s -xc test/common
	rm -f test/tmp_$*.c

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	rm -f $(TESTS)

clean:
	rm -f 9cc *.o *~ tmp* test/tmp_*

.PHONY: test clean
