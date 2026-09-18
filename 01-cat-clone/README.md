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

- `argc` and `argv`: how command-line arguments are represented and processed.
- File descriptors and POSIX calls: using `open()`, `read()`, `write()`, and
  `close()` to work with raw file data.
- Buffers and `ssize_t`: copying data in chunks and handling byte counts and
  system-call errors.
- `errno` and `perror()`: reporting useful errors when an operation fails.
- Pointer arithmetic and ownership: continuing partial writes and closing only
  file descriptors opened by the program.

## Benchmarking

To compare buffer sizes, I used the same 512 MiB test file, compiled with
optimisation enabled, and redirected output to `/dev/null`. Bash's built-in
`time` command measured five runs for each buffer size.

The benchmark command, run from the repository root, was:

```sh
g++ -O2 -std=c++17 -Wall -Wextra -pedantic \
    01-cat-clone/cat-clone.cpp -o /tmp/samns-cat

# For each value (1024, 4096, 65536, and 1048576), change BUFFER_SIZE
# in cat-clone.cpp, compile, and run five trials:
g++ -O2 -std=c++17 -Wall -Wextra -pedantic \
  01-cat-clone/cat-clone.cpp -o /tmp/samns-cat
for i in 1 2 3 4 5; do
  time /tmp/samns-cat /tmp/cat-benchmark.bin > /dev/null
done

for i in 1 2 3 4 5; do
    time cat /tmp/cat-benchmark.bin > /dev/null
done
```

Average results from five runs (real, user, sys):

```text
buffer       real     user     sys
1024         0.231s   0.038s   0.192s
4096         0.105s   0.011s   0.093s
65536        0.081s   0.001s   0.080s
1048576      0.119s   0.001s   0.117s
system cat   0.020s   0.000s   0.020s
```

The 64 KiB buffer was fastest for this clone and test file. The 1 MiB buffer
was slower, suggesting that increasing the buffer indefinitely does not always
improve performance. System `cat` was much faster overall. Let's look at which
system calls are being made.

### System-call comparison

Using `strace -c` shows why system `cat` is faster. The clone performs roughly
131,000 `read()` calls and 131,000 `write()` calls. System `cat` uses Linux's
`splice()` system call instead, reducing the amount of data that passes
through user space.

The clone:

```text
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 59.69    0.276400           2    131077           read
 40.30    0.186623           1    131073           write
  0.00    0.000004           0         6           close
  0.00    0.000000           0         6           fstat
  0.00    0.000000           0        22           mmap
  0.00    0.000000           0         6           mprotect
  0.00    0.000000           0         3           brk
  0.00    0.000000           0         1         1 ioctl
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
  0.00    0.000000           0         1           arch_prctl
  0.00    0.000000           0         1           futex
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         6           openat
  0.00    0.000000           0         5           newfstatat
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         2           getrandom
  0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.463027           1    262215         2 total
```

System `cat`:

```text
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 98.44    0.019700           9      2049           splice
  0.50    0.000100           3        31        12 openat
  0.41    0.000082           3        22           mmap
  0.26    0.000053           2        21           fstat
  0.22    0.000044           2        21           close
  0.07    0.000014          14         1           pipe2
  0.05    0.000011          11         1           fcntl
  0.04    0.000008           8         1           fadvise64
  0.00    0.000000           0         3           read
  0.00    0.000000           0         3           mprotect
  0.00    0.000000           0         3           brk
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
  0.00    0.000000           0         1           arch_prctl
  0.00    0.000000           0         1           futex
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         2           newfstatat
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         2           getrandom
  0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.020012           9      2168        13 total
```

Commands used:

```sh
strace -c /tmp/samns-cat /tmp/cat-benchmark.bin > /dev/null
strace -c cat /tmp/cat-benchmark.bin > /dev/null
```

## Next Stage: `splice()`

The next optimisation would be to experiment with `splice()`, a Linux-specific
system call that transfers data between file descriptors without copying it
through a user-space buffer. This could reduce the number of `read()` and
`write()` calls, but it would require a larger refactor and would not support
all of the same input and output cases as the current portable approach.
