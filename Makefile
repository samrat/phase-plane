all: main.c
	gcc -g -Wall --pedantic -std=c11 -o pplane main.c `sdl2-config --cflags --libs` -lGLEW -lGL -lm
