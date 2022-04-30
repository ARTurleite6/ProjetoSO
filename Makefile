all: server client monitor

server: bin/sdstored

client: bin/sdstore

monitor : bin/monitor

bin/monitor : obj/monitor.o
	gcc -g obj/monitor.o -o bin/monitor

obj/monitor.o : src/monitor.c
	gcc -Wall -g -c src/monitor.c -o obj/monitor.o

bin/sdstored: obj/sdstored.o
	gcc -g obj/sdstored.o -o bin/sdstored

obj/sdstored.o: src/sdstored.c
	gcc -Wall -g -c src/sdstored.c -o obj/sdstored.o

bin/sdstore: obj/sdstore.o
	gcc -g obj/sdstore.o -o bin/sdstore

obj/sdstore.o: src/sdstore.c
	gcc -Wall -g -c src/sdstore.c -o obj/sdstore.o

clean:
	rm obj/* tmp/* bin/{sdstore,sdstored,monitor}
