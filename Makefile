all: cache-sim extra-credit

run:
	./cache-sim

cache-sim.cpp:
	g++ -Wall -Werror -g cache-sim.cpp -o cache-sim

extra-credit.cpp:
	g++ -Wall -Werror -g extra-credit.cpp -o extra-credit

clean:
	rm -rf *.o cache-sim extra-credit caches Makefile~ cache-sim.cpp~ cache-sim.h~ out.txt
