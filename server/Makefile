all: dummy

server.o: server.cpp server.h
	./extract_commands.py
	gcc -Wall -c -g server.cpp

main.o: main.cpp
	gcc -Wall -c -g main.cpp 

dummy: main.o server.o
	gcc -o dummy main.o server.o -lzmq -lc++

clean:
	rm *.o
	rm *.include
	rm help.toml
	rm dummy
