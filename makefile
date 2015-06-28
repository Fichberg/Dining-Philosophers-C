philosopher: philosopher.o
	gcc -pthread -o philosopher philosopher.o -lgmp

philosopher.o: philosopher.c
	gcc -c philosopher.c -Wall -pedantic -ansi -g

clean:
	rm -rf *.o
	rm -rf *~
	rm philosopher