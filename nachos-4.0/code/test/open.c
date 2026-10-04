#include "syscall.h"

int main()
{
    PrintInt(Open("../test/printint.c"));
    PrintInt(Open("../test/no-such-file-for-open-test"));
    PrintInt(Open("../test/printint.c"));
    Halt();
    return 0;
}
