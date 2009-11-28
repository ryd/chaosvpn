CC = gcc
COPT = -O2 -Wall
LIB = -lcurl -ll
LEX=flex
YACC=yacc

STRINGOBJ=string/string_clear.o string/string_concatb.o string/string_concat.o string/string_free.o string/string_get.o string/string_init.o
OBJ = tinc.o fs.o parser.o tun.o http.o main.o y.tab.o lex.yy.o settings.o daemon.o $(STRINGOBJ)

NAME = chaosvpn

$(NAME): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB)

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

splint:
	splint +posixlib +allglobals -type -mayaliasunique *.[ch]
