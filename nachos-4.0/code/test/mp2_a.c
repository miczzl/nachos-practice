#include "syscall.h"

volatile int values[40] = {111};

int main()
{
    int i;
    if (values[0] != 111 || values[39] != 0) {
        PrintInt(-111);
        Exit(1);
    }
    for (i = 0; i < 1500; i++) {
        values[39] = i + 111;
        if (values[0] != 111 || values[39] != i + 111) {
            PrintInt(-111);
            Exit(1);
        }
    }
    PrintInt(111);
    Exit(0);
    return 0;
}
