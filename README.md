# FreeRTOS-Amiga

Simplified Unix-like operating system for Amiga 500 (and alike) computer.
It has modern kernel design based on current BSD family operating systems.
It's main purpose is a teaching aid for an advanced OS course.

Programming environment is based on:
 
 - patched [fs-uae](https://fs-uae.net) simulator
 - standard GNU toolchain i.e.: `gcc`, `binutils` and `gdb`

Main assumptions:

 - kernel should fit in less than 64kB of RAM
 - simple AmigaOS-style executable files
 - no virtual memory management (process spawning by `vfork`)
 - simple Minix-like filesystem
 - basic set of tools in `/bin` directory
 - terminal driver
 - single threaded processes
 - sbrk based userspace memory allocation
 - simple locking rules (no fine-grained locking systemwide)
 - single user
 - pipes
 - signals

Provides drivers for:

 - floppy disk driver
 - terminal display
 - serial port
 - keyboard
 - mouse
 - floppy disk drive
 - various timers
