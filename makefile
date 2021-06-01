main: main.o
	gcc -o run main.o -lpthread

main.o: main.c
	cc -c main.c

clean:
	rm -f main.o