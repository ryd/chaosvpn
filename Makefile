CC = gcc
COPT = -O2 -Wall
LIB = -lcurl

STRING_OBJ = string/string_clear.o string/string_concatb.o string/string_concat.o string/string_free.o string/string_get.o string/string_init.o
OBJ = tinc.o fs.o parser.o tun.o http.o main.o $(STRING_OBJ)

NAME = chaosvpn

$(NAME): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB)

$(STRING_OBJ):
	cd string && $(MAKE)

%.o: %.c
	$(CC) $(COPT) -c $<
       
clean:
	rm *.o $(NAME) 
