#include "emcoreapp.h"


static void main()
{
    cputs(2, "Memory map:\n");
    malloc_walk(NULL, (void*)2);
    cputc(2, '\n');
}


EMCORE_APP_HEADER("Memory allocation dumper", main, 127)
