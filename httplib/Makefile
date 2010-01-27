CC=gcc
INCLUDES=-I/usr/local/include
LIBDIRS=-L/usr/local/lib
CFLAGS=-std=c99 -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -D_FILE_OFFSET_BITS=64 -O0 -Wall -g
LIB=

STRINGSRC=../string/string_clear.c ../string/string_concatb.c ../string/string_concat_sprintf.c ../string/string_putc.c ../string/string_putint.c ../string/string_concat.c ../string/string_free.c ../string/string_get.c ../string/string_init.c ../string/string_equals.c ../string/string_move.c ../string/string_initfromstringz.c ../string/string_lazyinit.c
SRC = http_get.c http_parseurl.c $(STRINGSRC)
STRINGOBJ=$(patsubst %.c,%.o,$(STRINGSRC))
OBJ=$(patsubst %.c,%.o,$(SRC))

NAME = test_httplib

$(NAME): test.o $(OBJ)
	$(CC) -o $@ test.o $(OBJ) $(LIB) $(LIBDIRS)

%.o: %.c
	$(CC) $(CFLAGS) -o $(patsubst %.c,%.o,$<) -c $<

