#include "BitonicSortSample.h"
#include <iostream>

int main()
{
    SignalScatter::BitonicSortSample bss(32);

    int loopCount = 1;
    for (int i = 0; i < loopCount; i++)
    {
        bss.Run();
    }
}