#include "syscall.h"

volatile int cleared;

int main()
{
    if (cleared != 0) {
        PrintInt(-333);
        Exit(1);
    }

    // A later program reusing these frames must see zeroed BSS again.
    cleared = 9;
    return 0;
}
