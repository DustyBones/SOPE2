all: primes.c
	mkdir -p bin
	gcc  primes.c -o bin/primes -Wall -lm -pthread
