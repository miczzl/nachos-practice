#include "syscall.h"

int main()
{
    char message[] = "abc\n";
    OpenFileId id = Open("/tmp/nachos-write-test.txt");

    if (id < 0) {
        PrintInt(-1);
        Halt();
    }

    int written = Write(message, sizeof(message) - 1, id);
    PrintInt(written);  // should print 4

    char message2[] = "Hi\n";
    PrintInt(Write(message2, 3, ConsoleOutput)); // print Hi and then print 3
    PrintInt(Write(message2, 3, -1));            // print -1
    Halt();
    return 0;
}