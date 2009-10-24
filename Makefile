CC = gcc
COPT = -O2 
LIB = -lm

OBJ = main.o tun.o

NAME = chaosvpn

$(NAME): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB)
	strip $@

%.o: %.c
	$(CC) $(COPT) -c $<
       
clean:
	rm *.o $(NAME) 
