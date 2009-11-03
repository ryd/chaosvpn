CC = gcc
COPT = -O2 -Wall
LIB = -lcurl

STRING_OBJ = string_clear.o string_concatb.o string_concat.o string_free.o string_get.o string_init.o
OBJ = tinc.o fs.o parser.o tun.o http.o main.o $(STRING_OBJ)

NAME = chaosvpn

$(NAME): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB)

%.o: %.c
	$(CC) $(COPT) -c $<
       
clean:
	rm *.o $(NAME) 
