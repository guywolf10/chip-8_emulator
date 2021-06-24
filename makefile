emulator: main.o hardware.h
	gcc -g -Wall -pedantic main.o -o emulator

main.o: main.c hardware.h
	gcc -c -Wall -pedantic main.c -o main.o