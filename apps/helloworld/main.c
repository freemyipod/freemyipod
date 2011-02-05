#include "emcoreapp.h"


static void main()
{
    cputc(3, "Hello, world!\n");
}


EMCORE_APP_HEADER("Hello world", main, 127)
