#include "syscall.h"

volatile int values[40] = {222};

int main()
{
    int i;
    if (values[0] != 222 || values[39] != 0) {
        PrintInt(-222);
        Exit(1);
    }
    for (i = 0; i < 3000; i++) {
        values[39] = i + 222;
        if (values[0] != 222 || values[39] != i + 222) {
            PrintInt(-222);
            Exit(1);
        }
    }
    PrintInt(222);
    Exit(0);
    return 0;
}
