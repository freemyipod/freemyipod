#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include "syscallapi.h"
#include "syscallwrappers.h"


#define EMBIOS_APP_HEADER(threadnamestr, stacksizebytes, mainfunc, threadprio)                    \
    uint32_t __embios_thread_stack[stacksizebytes >> 2];                                          \
    char __embios_thread_name[] = threadnamestr;                                                  \
    struct embios_app_header                                                                      \
    {                                                                                             \
        char signature[8];                                                                        \
        int version;                                                                              \
        void* baseaddr;                                                                           \
        int size;                                                                                 \
        uint32_t crc32;                                                                           \
        void* stackaddr;                                                                          \
        int stacksize;                                                                            \
        void* entrypoint;                                                                         \
        char* threadname;                                                                         \
        int threadtype;                                                                           \
        int threadpriority;                                                                       \
    } __embios_executable_hdr __attribute__((section(".execheader"))) =                           \
    {                                                                                             \
        .signature = "emBIexec",                                                                  \
        .version = 0,                                                                             \
        .baseaddr = &__embios_executable_hdr,                                                     \
        .size = -1,                                                                               \
        .crc32 = 0,                                                                               \
        .stackaddr = __embios_thread_stack,                                                       \
        .stacksize = stacksizebytes,                                                              \
        .entrypoint = mainfunc,                                                                   \
        .threadname = __embios_thread_name,                                                       \
        .threadtype = 0,                                                                          \
        .threadpriority = threadprio                                                              \
    };                                                                                            \
    struct embios_syscall_table* __embios_syscall;


#define embios_init()                                                                             \
    asm volatile("swi\t2\n\tldr\tr1,\t=__embios_syscall\n\tstr\tr0,\t[r1]\n\t"                    \
             ::: "r0", "r1", "r2", "r3", "r12", "lr", "cc", "memory");

