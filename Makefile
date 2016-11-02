CC?=gcc
INCLUDES?=-I/usr/local/include
LIBDIRS?=-L/usr/local/lib
LIB?=-lz -lcrypto

OS=$(shell uname)
ifneq (,$(findstring BSD,$(OS)))
	# hopefully FreeBSD, OpenBSD, NetBSD
	CFLAGS+=-DBSD -std=c99 -D_FILE_OFFSET_BITS=64 -O2 -Wall -g $(INCLUDES)
	PREFIX?=/usr/local
	TINCDIR?=/usr/local/etc/tinc
else
ifneq (,$(findstring Darwin,$(OS)))
	# Apple OSX / Darwin
	CFLAGS+=-DBSD -std=c99 -D_FILE_OFFSET_BITS=64 -O2 -Wall -g $(INCLUDES)
	PREFIX?=/usr/local
	TINCDIR?=/usr/local/etc/tinc
else
	# Linux by default
	CFLAGS+=-std=c99 -D_POSIX_C_SOURCE=2 -D_DEFAULT_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -Wall -g $(INCLUDES)
	PREFIX?=/usr
	TINCDIR?=/etc/tinc
endif
endif
ifneq (,$(findstring mingw,$(CC)))
	CFLAGS+=-DWIN32
	LIB+=-lws2_32 -lgdi32
endif

LEX=flex
YACC?=yacc

CFLAGS += -DPREFIX="\"$(PREFIX)\"" -DTINCDIR="\"$(TINCDIR)\""

STRINGSRC=string/string_clear.c string/string_concatb.c string/string_concat_sprintf.c string/string_putc.c string/string_putint.c string/string_concat.c string/string_free.c string/string_get.c string/string_init.c string/string_equals.c string/string_move.c string/string_initfromstringz.c string/string_lazyinit.c string/string_read.c string/string_hexdump.c
HTTPLIBSRC=httplib/http_get.c httplib/http_parseurl.c
SRC = tinc.c fs.c parser.c tun.c y.tab.c lex.yy.c config.c strnatcmp.c daemon.c crypto.c ar.c uncompress.c log.c pidfile.c addrmask.c $(STRINGSRC) $(HTTPLIBSRC)
HEADERS = chaosvpn.h list.h strnatcmp.h version.h y.tab.h addrmask.h httplib/httplib.h string/string.h
STRINGOBJ=$(patsubst %.c,%.o,$(STRINGSRC))
HTTPLIBOBJ=$(patsubst %.c,%.o,$(HTTPLIBSRC))
OBJ=$(patsubst %.c,%.o,$(SRC))

NAME?=chaosvpn
GITDEBVERSION=$(shell debian/scripts/calcdebversion )

all: $(NAME)

$(NAME): main.o $(OBJ) $(HEADERS)
	$(CC) $(LDFLAGS) -o $@ main.o $(OBJ) $(LIB) $(LIBDIRS)

test_addrmask: test_addrmask.o $(OBJ) $(HEADERS)
	$(CC) $(LDFLAGS) -o $@ test_addrmask.o $(OBJ) $(LIB) $(LIBDIRS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $(patsubst %.c,%.o,$<) -c $<

lex.yy.o: lex.yy.c y.tab.h

y.tab.c y.tab.h: cvconf.y
	$(YACC) -d cvconf.y

lex.yy.c: cvconf.l
	$(LEX) cvconf.l

clean:
	rm -f *.o y.tab.c y.tab.h lex.yy.c string/*.o httplib/*.o $(NAME) test_addrmask

CHANGES:
	[ -e .git/HEAD -a -n "$(shell which git)" ] && git log >CHANGES || true

baseinstall:
	#strip $(NAME)
	install -m 0755 -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 0644 man/chaosvpn.1 $(DESTDIR)$(PREFIX)/share/man/man1/
	install -m 0755 -d $(DESTDIR)$(PREFIX)/share/man/man5
	install -m 0644 man/chaosvpn.conf.5 $(DESTDIR)$(PREFIX)/share/man/man5/

	install -m 0755 -d $(DESTDIR)$(PREFIX)/sbin
	install -m 0755 $(NAME) $(DESTDIR)$(PREFIX)/sbin/

linuxinstall: baseinstall
	@if [ ! -e $(DESTDIR)$(TINCDIR)/chaosvpn.conf ] ; then \
		install -m 0755 -d $(DESTDIR)$(TINCDIR) ; \
		install -m 0600 chaosvpn.conf $(DESTDIR)$(TINCDIR)/chaosvpn.conf ; \
		echo "Created config File $(TINCDIR)/chaosvpn.conf"; \
	fi
	@if [ ! -e $(DESTDIR)$(TINCDIR)/warzone.conf ] ; then \
		install -m 0755 -d $(DESTDIR)$(TINCDIR) ; \
		install -m 0600 warzone.conf $(DESTDIR)$(TINCDIR)/warzone.conf ; \
		echo "Created config File $(TINCDIR)/warzone.conf"; \
	fi

install: linuxinstall

bsdinstall: baseinstall
	install -m 0755 freebsd.rc.d $(DESTDIR)$(PREFIX)/etc/rc.d/chaosvpn

	@if [ ! -e $(DESTDIR)$(TINCDIR)/chaosvpn.conf ] ; then \
		install -m 0755 -d $(DESTDIR)$(TINCDIR) ; \
		install -m 0600 chaosvpn.conf.freebsd $(DESTDIR)$(TINCDIR)/chaosvpn.conf ; \
		echo "Created config File $(TINCDIR)/chaosvpn.conf from BSD template"; \
	fi

appleinstall: baseinstall
	# **FIXME** NEED REPLACEMENT - install -m 0755 freebsd.rc.d $(DESTDIR)$(PREFIX)/etc/rc.d/chaosvpn

	@if [ ! -e $(DESTDIR)$(TINCDIR)/chaosvpn.conf ] ; then \
		install -m 0755 -d $(DESTDIR)$(TINCDIR) ; \
		install -m 0600 chaosvpn.conf.apple $(DESTDIR)$(TINCDIR)/chaosvpn.conf ; \
		echo "Created config File $(TINCDIR)/chaosvpn.conf from Apple template"; \
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

