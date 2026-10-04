#include "syscall.h"

int main()
{
    char buffer[32];
    int count;
    OpenFileId id = Open("../test/read_fixture.txt");

    if (id < 0) {
        PrintInt(-1);
    } else {
        PrintInt(Read(buffer, 0, id));

        count = Read(buffer, sizeof(buffer), id);
        PrintInt(count);
        if (count > 0)
            Write(buffer, count, ConsoleOutput);

        PrintInt(Read(buffer, sizeof(buffer), id));
    }

    PrintInt(Read(buffer, 1, -1));

    count = Read(buffer, 1, ConsoleInput);
    PrintInt(count);
    if (count > 0)
        Write(buffer, count, ConsoleOutput);

    Halt();
    return 0;
}
