
CC = g++
LDFLAGS = -framework GLUT -framework OpenGL -lGLEW -lpng12
CFLAGS = -c

all:
	$(CC) $(LDFLAGS) main.cpp lib/png_reader.c -o bin/main

clean:
	rm -f *.o main
