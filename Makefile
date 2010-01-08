CC=gcc
INCLUDES=-I/usr/local/include
LIBDIRS=-L/usr/local/lib
CFLAGS=-O2 -Wall -g $(INCLUDES)
LIB=-lcurl
LEX=flex
YACC=yacc

STRINGOBJ=string/string_clear.o string/string_concatb.o string/string_concat_sprintf.o string/string_putc.o string/string_putint.o string/string_concat.o string/string_free.o string/string_get.o string/string_init.o
OBJ = tinc.o fs.o parser.o tun.o http.o main.o y.tab.o lex.yy.o settings.o daemon.o $(STRINGOBJ)

NAME = chaosvpn
GITVERSION=$(shell scripts/calcdebversion )

$(NAME): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB) $(LIBDIRS)

$(STRINGOBJ):
	cd string && make

%.o: %.c
	$(CC) $(COPT) -c $<
       
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
	splint +posixlib +allglobals -type -mayaliasunique *.[ch]

deb:
	[ -n "$(GITVERSION)" ] # check if gitversion is set
	if git status debian/changelog >/dev/null 2>/dev/null ; then \
		echo -e "\nERROR: uncommited changes in debian/changelog!\n       either commit or revert with 'git checkout debian/changelog'\n" ; \
		exit 1 ; \
	fi
	debchange --force-distribution --noquery --preserve --newversion $(GITVERSION) "Compiled GIT snapshot."
	debuild --no-tgz-check || true
	git checkout debian/changelog
	