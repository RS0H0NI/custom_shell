# wsh

`wsh` is a small command-line shell written in C. It runs commands found on
`PATH`, supports pipelines and sequential commands, and provides a few built-in
commands.

## Requirements

Build and run `wsh` in a POSIX-like environment with GCC and `make`, such as
Linux, macOS, or Windows Subsystem for Linux (WSL). It uses POSIX APIs including
`fork`, `exec`, and `pipe`, so it does not build as a native Windows shell
program.

## Build

From the project directory, run:

```sh
make
```

This builds the optimized `wsh` executable and a debug build named `wsh-dbg`.
To remove generated build files:

```sh
make clean
```

## Run

Start an interactive shell:

```sh
./wsh
```

At the `wsh>` prompt, enter a command. Use `exit` to leave the shell.

Run commands from a text file in batch mode:

```sh
./wsh commands.txt
```

You can also pipe input into the shell:

```sh
printf 'echo hello\nexit\n' | ./wsh
```

On Windows with WSL, open a WSL terminal and navigate to the project directory.
For a repository under `C:\Users\custom_shell`,
the corresponding WSL path is typically:

```sh
cd /mnt/c/Users/custom_shell
make
./wsh
```

If GCC and `make` are not installed in Ubuntu or Debian under WSL, install them
with:

```sh
sudo apt update
sudo apt install build-essential
```

## Supported syntax

Commands are separated by semicolons and run sequentially:

```sh
echo first; echo second
```

Use `|` to connect commands with a pipeline:

```sh
printf 'pear\napple\n' | sort
```

Arguments can be quoted with single or double quotes, and a backslash escapes
the next character outside single quotes:

```sh
echo "hello there"
echo 'text with spaces'
echo one\ two
```

Environment variables can be expanded with `$NAME`. Brace syntax such as
`${NAME}` is not supported:

```sh
echo "$HOME"
```

An input line whose first non-whitespace character is `#` is treated as a
comment.

Built-in commands:

- `cd [directory]` changes the shell's working directory. With no directory,
  it changes to `$HOME`.
- `env` prints the environment. `env NAME=value` sets or replaces an
  environment variable for subsequent commands in this shell.
- `exit` exits the shell.

Commands not recognized as built-ins are searched for using `PATH`. Absolute
paths to executables are also accepted.

## Current limitations

This is a minimal shell, not a replacement for Bash or another full-featured
shell. Input/output redirection, command substitution, job control, and
wildcard expansion are not implemented. Variable expansion supports `$NAME`
only (not `${NAME}`).
