SRC = pplane.c gl3w/gl3w.c
LIBS = `sdl2-config --cflags --libs` -lGL -lm -ldl
CFLAGS = -fsanitize=address -g -Wall --pedantic -std=c11
INCLUDES = -Igl3w/

all: $(SRC)
	gcc $(CFLAGS) -o pplane $(INCLUDES) $(SRC) $(LIBS)
