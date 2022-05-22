all:
	g++ -Isrc/include -Lsrc/lib -o main graphics.cpp -lmingw32 -lSDL2main -lSDL2
	