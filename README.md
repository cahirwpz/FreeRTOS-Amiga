# EduKer

Simplified Unix-like operating system for Amiga 500 (and alike) computer.
It has modern kernel design based on current BSD family operating systems.
It's main purpose is a teaching aid for an advanced OS course.

Programming environment is based on:
 
 - patched [fs-uae](https://fs-uae.net) simulator
 - standard GNU toolchain i.e.: `gcc`, `binutils` and `gdb`

Main assumptions:

 - kernel should fit in less than 64kB of RAM
 - AmigaOS-style executable files
 - Minix-like filesystem
 - basic set of tools in `/bin` directory
 - terminal driver
 - process spawning by `vfork`
 - single threaded processes
 - non-preemptible kernel
 - simple locking rules (no fine-grained locking systemwide)
 - single user
 - pipes
 - signals

If page-based MMU is available then:

 - simple virtual address space layout:
   - code, data and stack segments
   - sbrk based memory allocation
 - real `fork`
 - code segment sharing

Provides drivers for:

 - storage devices:
   - floppy drive
   - IDE hard drive
 - vt100 compatible console:
   - display
   - keyboard
   - mouse
 - serial port
 - various timers
