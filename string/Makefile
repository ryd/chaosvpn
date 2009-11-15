CC=cc
CFLAGS=-O2 -Wall

all: string_clear.o string_concat.o string_concatb.o string_get.o string_free.o string_init.o
	true

%.o: %.c
	$(CC) $(CFLAGS) -c $<

