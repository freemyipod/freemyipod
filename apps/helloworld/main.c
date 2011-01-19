#include "emcoreapp.h"


void main();
EMCORE_APP_HEADER("Hello world", main, 127)


void main()
{
    panic(PANIC_KILLTHREAD, "Hello, world!");
}
