Spielbub
========

![Spielbub](./screenshot.png?raw=true)

Yet another unfinished Gameboy emulator, written for fun and profit in C.

What does it do?
----------------

Right now you can run Super Mario Land and enter the game. No sound emulation or any other bells and whistles.

Controls are as follows:

*  Directions: WASD
*  Start/Select/A/B: IJKL

I should probably change these.

What do I need to compile it?
-----------------------------

Compilation has only been tested on OS X, your mileage may vary on other OS. Unixes should be fine though. You'll need the following libraries to compile:

* libsdl
* libcheck

### Building

Install `nix`

```shell
$ nix-shell # tested on macOS and linux
[nix-shell]$ premake4 gmake --cc="clang"
[nix-shell]$ make -j
[nix-shell]$ ./Spielbub <rom.bin>
```

Good luck.
