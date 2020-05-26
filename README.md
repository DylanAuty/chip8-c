# chip8-c

A Chip-8 emulator written in C, using ncurses for the display. Written as a little side project, so there are some quick and dirty design decisions in here. It wuold have been better to use a struct for the processor, or to use C++ and make the whole thing object-oriented... but I didn't. Oh well.

Although it's more efficient to use a normal `int` for most things, I've decided to implement everything "internal" to the Chip-8 as being the exact width it should be. Whether this is a _good_ decision is another matter, but I felt it was better to be as accurate as I could to the specification, at the expense of a little bit of overhead for the host processor to fiddle around with whatever slicing and shifting operations it needs to do to make this work.

## What's Chip-8?
See the [CHIP-8 Wikipedia article](https://en.wikipedia.org/wiki/CHIP-8) and then read [Cowgod's Chip-8 reference page](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM). It's not a processor, I know, but since it behaves like one I'm calling this project an "emulator" anyway.

## Building
Requires ncurses. Simply do `make`. `make clean` works too, but it's a bit redundant when there's only one file.

## Running
* `./chip8` if you don't want to run a program - a bit useless but hey, up to you.
* `./chip8 /path/to/program.ch8` if you do. I don't know who owns the testing programs I used, so I won't put them here, but a very quick google search will turn up quite a few.

Program will start by drawing the frame that represents the display. To actually begin execution of the processor, press any key.

You should see a complete frame all the way around the display on startup. If you don't, kill the program, make the window bigger, and try again. Resizing while running isn't currently implemented.

## Issues
Programs that require held down key presses don't work - accessing keyup events is apparently something that requires root permissions, so until I put a workaround in, this is broken.

