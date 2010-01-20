CC=gcc
INCLUDES=-I/usr/local/include
LIBDIRS=-L/usr/local/lib
CFLAGS=-std=c99 -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -Wall -g $(INCLUDES)
LIB=-lcurl -lcrypto
LEX=flex
YACC=yacc

STRINGSRC=string/string_clear.c string/string_concatb.c string/string_concat_sprintf.c string/string_putc.c string/string_putint.c string/string_concat.c string/string_free.c string/string_get.c string/string_init.c
SRC = tinc.c fs.c parser.c tun.c http.c y.tab.c lex.yy.c settings.c daemon.c verify.c $(STRINGSRC)
STRINGOBJ=$(patsubst %.c,%.o,$(STRINGSRC))
OBJ=$(patsubst %.c,%.o,$(SRC))

NAME = chaosvpn
GITDEBVERSION=$(shell debian/scripts/calcdebversion )

$(NAME): main.o $(OBJ)
	$(CC) -o $@ main.o $(OBJ) $(LIB) $(LIBDIRS)

%.o: %.c
	$(CC) $(CFLAGS) -o $(patsubst %.c,%.o,$<) -c $<

lex.yy.o: lex.yy.c y.tab.h

y.tab.c y.tab.h: cvconf.y
	$(YACC) -d cvconf.y

lex.yy.c: cvconf.l
	$(LEX) cvconf.l

clean:
	rm -f *.o y.tab.c y.tab.h lex.yy.c string/*.o $(NAME)

CHANGES:
	[ -e .git/HEAD ] && git log >CHANGES

install:
	install -m 0755 chaosvpn $(DESTDIR)/usr/sbin/
	if [ ! -e $(DESTDIR)/etc/tinc/chaosvpn.conf ] ; then \
		install -m 0600 chaosvpn.conf $(DESTDIR)/etc/tinc/ ; \
	fi

splint:
	splint +posixlib +allglobals -type -mayaliasunique -predboolint \
		-retvalint $(CFLAGS) main.c $(SRC)

deb:
	[ -n "$(GITDEBVERSION)" ] # check if gitversion is set
	dpkg-checkbuilddeps
	if git status debian/changelog >/dev/null 2>/dev/null ; then \
		echo -e "\nERROR: uncommited changes in debian/changelog!\n       either commit or revert with 'git checkout debian/changelog'\n" ; \
		exit 1 ; \
	fi
	debchange --noquery --preserve --force-bad-version --newversion $(GITDEBVERSION) "Compiled GIT snapshot."
	debuild --no-tgz-check || true
	git checkout debian/changelog
	