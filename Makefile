all: cache-sim

run:
	./cache-sim

caches.cpp:
	g++ -Wall -Werror -g caches.cpp -o cache-sim

clean:
	rm -rf *.o cache-sim caches Makefile~ caches.cpp~ caches.h~ out.txt
