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
build also builds `../bin/coff2noff`. `-e` selects the user program and skips
NachOS's interactive console self-test. Without `-e`, NachOS keeps its
original behavior of running `../test/halt`.

The test program should call `Halt()` when done; this starter kernel does not
yet implement the `Exit` system call. Test outputs (`*.o`, `*.coff`, and
`*.noff`) are ignored by Git.
