#include "emcoreapp.h"


static void main()
{
    cputs(3, "Hello, world!\n");
}


EMCORE_APP_HEADER("Hello world", main, 127)
