all : trie.o main.o
	gcc -o trie -g main.o trie.o

main.o : main.c
	gcc -c -g main.c

trie.o : trie.c trie.h
	gcc -c -g trie.c

clean :
	rm trie *.o
