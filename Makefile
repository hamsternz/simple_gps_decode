COPTS=-Wall -pedantic -O4
LOPTS=-lm

example/main : example/main.o gps_parse.o
	gcc -o example/main example/main.o gps_parse.o $(LOPTS)

example/main.o : example/main.c gps_parse.h
	gcc -c -o example/main.o example/main.c $(COPTS)

gps_parse.o: gps_parse.c gps_parse.h
	gcc -c gps_parse.c $(COPTS)

clean:
	rm example/main example/main.o gps_parse.o 
