CC?=gcc
INCLUDES?=-I/usr/local/include
LIBDIRS?=-L/usr/local/lib
CFLAGS?=-std=c99 -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -Wall -g $(INCLUDES)
LIB?=-lz -lcrypto
LEX?=flex
YACC?=yacc

PREFIX?=/usr
TINCDIR?=/etc/tinc

STRINGSRC=string/string_clear.c string/string_concatb.c string/string_concat_sprintf.c string/string_putc.c string/string_putint.c string/string_concat.c string/string_free.c string/string_get.c string/string_init.c string/string_equals.c string/string_move.c string/string_initfromstringz.c string/string_lazyinit.c string/string_read.c
HTTPLIBSRC=httplib/http_get.c httplib/http_parseurl.c
SRC = tinc.c fs.c parser.c tun.c y.tab.c lex.yy.c settings.c strnatcmp.c daemon.c crypto.c ar.c uncompress.c $(STRINGSRC) $(HTTPLIBSRC)
HEADERS = ar.h crypto.h daemon.h fs.h list.h main.h parser.h settings.h strnatcmp.h tinc.h tun.h uncompress.h version.h y.tab.h httplib/httplib.h string/string.h
STRINGOBJ=$(patsubst %.c,%.o,$(STRINGSRC))
HTTPLIBOBJ=$(patsubst %.c,%.o,$(HTTPLIBSRC))
OBJ=$(patsubst %.c,%.o,$(SRC))

NAME?=chaosvpn
GITDEBVERSION=$(shell debian/scripts/calcdebversion )

$(NAME): main.o $(OBJ) $(HEADERS)
	$(CC) -o $@ main.o $(OBJ) $(LIB) $(LIBDIRS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $(patsubst %.c,%.o,$<) -c $<

lex.yy.o: lex.yy.c y.tab.h

y.tab.c y.tab.h: cvconf.y
	$(YACC) -d cvconf.y

lex.yy.c: cvconf.l
	$(LEX) --yylineno cvconf.l

clean:
	rm -f *.o y.tab.c y.tab.h lex.yy.c string/*.o httplib/*.o $(NAME)

CHANGES:
	[ -e .git/HEAD ] && git log >CHANGES

install:
	strip $(NAME)
	install -m 0644 man/chaosvpn.1 $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 0644 man/chaosvpn.conf.5 $(DESTDIR)$(PREFIX)/share/man/man5

	install -m 0755 $(NAME) $(DESTDIR)$(PREFIX)/sbin/
	@if [ ! -e $(DESTDIR)$(TINCDIR)/$(NAME).conf ] ; then \
		install -m 0600 chaosvpn.conf $(DESTDIR)$(TINCDIR)/$(NAME).conf ; \
		echo "Created config File $(TINCDIR)/chaosvpn.conf"; \
	fi

splint:
	splint +posixlib +allglobals -type -mayaliasunique -predboolint \
		-retvalint $(CFLAGS) main.c $(SRC)

deb:
	[ -n "$(GITDEBVERSION)" ] # check if gitversion is set
	dpkg-checkbuilddeps
	@if git commit --dry-run -- debian/changelog >/dev/null 2>/dev/null ; then \
		echo -e "\nERROR: uncommited changes in debian/changelog!\n       either commit or revert with 'git checkout debian/changelog'\n" ; \
		exit 1 ; \
	fi
	@if git commit --dry-run -- version.h >/dev/null 2>/dev/null ; then \
		echo -e "\nERROR: uncommited changes in version.h!\n       either commit or revert with 'git checkout version.h'\n" ; \
		exit 1 ; \
	fi
	debchange --noquery --preserve --force-bad-version --newversion $(GITDEBVERSION) "Compiled GIT snapshot."
	echo "#define VERSION \"$(GITDEBVERSION)\"" >version.h
	debuild --no-tgz-check || true
	git checkout -- debian/changelog version.h
	@echo -e "\nDebian build successfull.";

