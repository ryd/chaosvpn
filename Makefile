CC = gcc
COPT = -O2 -Wall
LIB = -lcurl -ll
LEX=flex
YACC=yacc

OBJ = tinc.o fs.o parser.o tun.o http.o main.o y.tab.o lex.yy.o settings.o

NAME = chaosvpn

$(NAME): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB)

%.o: %.c
	$(CC) $(COPT) -c $<
       
lex.yy.o: lex.yy.c y.tab.h

lex.yy.o y.tab.o: chaosvpn.h

y.tab.c y.tab.h: cvconf.y
	$(YACC) -d cvconf.y

lex.yy.c: cvconf.l
	$(LEX) cvconf.l

clean:
	rm -f *.o y.tab.c y.tab.h lex.yy.c $(NAME) 

splint:
	splint +posixlib +allglobals -type -mayaliasunique *.[ch]
