# phase-plane

A program for phase plane analysis. **This is a work-in-progress.**

![phase-plane screenshot](http://i.imgur.com/AmzXFsz.png)

## Building

### Linux
You will need to have SDL2 installed. Once you have that, just run
`make`. Then, run `./pplane` to run.

### Windows
You will need to have MinGW-w64. The mingw Makefile(`Makefile.mingw`)
dynamically links against SDL2, and it expects there to be a `sdl/`
folder that on *nix you would obtain as follows:

~~~shell
 	 wget http://libsdl.org/release/SDL2-devel-2.0.4-mingw.tar.gz
 	 tar xvf SDL2-devel-2.0.4-mingw.tar.gz
 	 cp SDL2-2.0.4/x86_64-mingw32 sdl
~~~

I don't have enough Windows-command-line-fu to automate this step, so
you'll need to do this step manually.

After you have the `sdl/` folder, you can run `make -f Makefile.mingw`
to get the exe in the `build/` folder.

## Credits

The original `pplane` for Matlab by John C. Polking, which inspired
this project, can be found
[here](http://math.rice.edu/~dfield/index.html).
