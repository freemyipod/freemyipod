#include "embiosapp.h"


void main();
EMBIOS_APP_HEADER("Test application", 0x4000, main, 127)


void main()
{
    panic(PANIC_KILLTHREAD, "Hello, world!");
}
