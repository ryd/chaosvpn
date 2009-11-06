CC = gcc
COPT = -O2 -Wall
LIB = -lcurl

OBJ = tinc.o fs.o parser.o tun.o http.o main.o

NAME = chaosvpn

$(NAME): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB)

%.o: %.c
	$(CC) $(COPT) -c $<
       
clean:
	rm -f *.o $(NAME) 

splint:
	splint +posixlib +allglobals -type -mayaliasunique *.[ch]
