all: main.c gl3w/gl3w.c
	gcc -g -Wall --pedantic -std=c11 -o pplane -Igl3w/ gl3w/gl3w.c main.c `sdl2-config --cflags --libs` -lGL -lm -ldl
