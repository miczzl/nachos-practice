# Running user-program tests

Use the `nachos-dev` container. The MIPS compiler is bundled at the project
root, and `make` uses it automatically for programs in this directory.

For a new `foo.c` that includes `syscall.h`:

```sh
cd /workspace/nachos-4.0/code/test
make foo.noff

cd ../userprog
make
./nachos -e ../test/foo.noff
```

For the current `PrintInt` test, replace `foo` with `printint`. The first
build also builds `../bin/coff2noff`. Each `-e` selects another user program;
all selected programs get their own NachOS thread and address space.
Specifying `-e` skips the interactive console self-test. Without `-e`,
NachOS runs the interactive self-tests before starting `../test/halt`.

Use `Exit(status)` or return from `main()` to end one program and release its
frames. `Halt()` stops the whole simulated machine, including other programs.
Test outputs (`*.o`, `*.coff`, and `*.noff`) are ignored by Git.

For the MP2 tests, trace, and implementation notes, see
[the MP2 report](../../../docs/MP2.md).
