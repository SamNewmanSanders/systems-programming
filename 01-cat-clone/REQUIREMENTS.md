# Cat Clone Requirements

Implement a command-line program that reproduces the basic behavior of Unix
`cat`.

## Interface

- The program must accept zero or more command-line operands.
- With no operands, read from standard input.
- Each operand must be treated as a filename and processed from left to right.
- The operand `-` must represent standard input (use ctrl+D to finish input).
- The option `--help` must print usage information to standard output and exit.

## Required functionality

- Copy each input byte unchanged to standard output.
- Concatenate multiple input files without adding or removing content.
- Preserve the order of the operands.
- Continue processing later operands when possible if one file cannot be opened.
- Write a useful error message to standard error for unreadable or missing files.
- Return `0` when all requested inputs are processed successfully; otherwise,
  return a nonzero status.
- Return `0` after successfully displaying help information.

## Examples

```sh
# Read from standard input
./cat-clone < input.txt

# Print one file
./cat-clone file1.txt

# Display usage information
./cat-clone --help

# Concatenate files in the given order
./cat-clone file1.txt file2.txt

# Read file1.txt, then piped stdin, then file2.txt
printf 'content from stdin\n' | ./cat-clone file1.txt - file2.txt
```
