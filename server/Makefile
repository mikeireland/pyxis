all: dummy

server.o: server.cpp server.h
	./extract_commands.py
	g++ -Wall -c -g server.cpp

main.o: main.cpp
	g++ -Wall -c -g main.cpp 

dummy: main.o server.o
	g++ -o dummy main.o server.o -lzmq -lc++

clean:
	rm *.o
	rm *.include
	rm help.toml
	rm dummy
