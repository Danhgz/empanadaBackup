bin/green:	src/NodoVerde/NodoVerde.cpp | bin
	g++ -g -Wall -Wextra src/NodoVerde/NodoVerde.cpp -lpthread -o bin/nodoVerde

bin:
	mkdir -p bin

.PHONY:	clean
clean:
	rm -rf bin *.o
