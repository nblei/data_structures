TARGET = main
OBJS = main.o rbtree.o
FLAGS = -g3 -Wall -Werror

$(TARGET) : $(OBJS)
	gcc $(FLAGS) -o $(TARGET) $(OBJS)

main.o : main.c
	gcc $(FLAGS) -c main.c

rbtree.o : rbtree.c rbtree.h
	gcc $(FLAGS) -c rbtree.c

clean :
	rm *.o main
