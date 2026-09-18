# samns-cat

A small C++ clone of the Unix `cat` command. It reads from files or standard input and copies the input to standard output in the order provided.

## Why build it?

This project is an exercise in systems programming. Instead of relying on
higher-level C++ streams for file copying, it uses POSIX system calls to work directly with file descriptors, buffers, and raw bytes.

## Usage

```sh
# Compile
g++ -std=c++17 -Wall -Wextra -pedantic cat-clone.cpp -o samns-cat

# Read a file
./samns-cat example.txt

# Read standard input
./samns-cat < example.txt

# Read multiple inputs in order
./samns-cat first.txt second.txt

# Use standard input at a specific position
printf 'stdin content\n' | ./samns-cat example.txt -
```

With no file arguments, the program reads standard input. The `-` operand
also means standard input. `--help` displays usage information.

## What I am learning

- `argc` and `argv`: `argv[0]` is the program name; operands begin at `argv[1]`.
- File descriptors: standard input and output are represented by descriptor numbers, while `open()` returns descriptors for files.
- POSIX system calls: `open()`, `read()`, `write()`, and `close()` operate on raw file data.
- Buffers: input is copied in fixed-size chunks rather than one character at a time.
- `ssize_t`: system calls use it to represent byte counts as well as `-1` for errors.
- `errno` and `perror()`: failed system calls provide an error description, and the source name makes the message useful.
- Pointer arithmetic: `buffer + totalWritten` points to the next unwritten byte when a `write()` call writes only part of the buffer.
- Resource ownership: descriptors opened by the program should be closed,
  while standard input should not be closed accidentally.
