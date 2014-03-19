Spielbub
========

Yet another unfinished Gameboy emulator, written for fun and profit in C.

What does it do?
----------------

Right now you can run Super Mario Land and enter the game. Input still glitches unfortunately. No sound emulation or any other bells and whistles.

Controls are as follows:

*  Directions: WASD
*  Start/Select/A/B: IJKL

I should probably change these.

What do I need to compile it?
-----------------------------

Compilation has only been tested on OS X, your mileage may vary on other OS. Unixes should be fine though. You'll need the following libraries to compile:

* libsdl

### Building

```bash
# premake4 gmake
# make
# ./Spielbub <rom.bin>
```

Good luck.
