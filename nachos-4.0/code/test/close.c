#include "syscall.h"

int main()
{
    char byte;
    OpenFileId first = Open("../test/read_fixture.txt");

    if (first < 0) {
        PrintInt(-1);
        Halt();
    }

    PrintInt(Close(first));                /* 1: valid open file */
    PrintInt(Read(&byte, 1, first));       /* -1: no longer open */
    PrintInt(Close(first));                /* 0: already closed */
    PrintInt(Close(ConsoleInput));         /* 0: reserved console id */
    PrintInt(Close(ConsoleOutput));        /* 0: reserved console id */
    PrintInt(Close(-1));                   /* 0: invalid id */

    OpenFileId second = Open("../test/read_fixture.txt");
    PrintInt(second);                     /* same slot can be reused */
    PrintInt(Close(second));               /* 1 */

    Halt();
    return 0;
}
