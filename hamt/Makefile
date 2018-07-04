OBJS = hamt.o main.o
TARGET = main
FLAGS = -g3 -Wall -Werror

main : $(OBJS)
	gcc $(FLAGS) -o $(TARGET) $(OBJS)

hamt.o : hamt.h hamt.c
	gcc $(FLAGS) -c hamt.c

main.o : main.c
	gcc $(FLAGS) -c main.c

clean :
	rm main *.o

