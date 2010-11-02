#include "syscallwrappers.h"


#define EMBIOS_APP_HEADER(threadnamestr, stacksizebytes, mainfunc, threadprio)                    \
    void __embios_init()                                                                          \
    {                                                                                             \
        asm volatile("swi\t2\n\tldr\tr3,\t=__embios_required_version\nldr\tr3,\t[r3]\n\t"         \
                     "ldr\tr2,\t[r0]\n\tcmp\tr3,\tr2\n\tldrls\tr1,\t[r0,#4]\n\tcmpls\tr1,\tr3\n\t"\
                     "movhi\tr0,\t#0\n\tldrhi\tr1,\t=__embios_incompatible_api_str\n\t"           \
                     "swihi\t1\n\tldr\tr1,\t=__embios_syscall\n\tstr\tr0,\t[r1]\n\t"              \
                 ::: "r0", "r1", "r2", "r3", "r12", "lr", "cc", "memory");                        \
        mainfunc();                                                                               \
    }                                                                                             \
    uint32_t __embios_thread_stack[stacksizebytes >> 2];                                          \
    const char __embios_thread_name[] = threadnamestr;                                            \
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
        const char* threadname;                                                                   \
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
        .entrypoint = __embios_init,                                                              \
        .threadname = __embios_thread_name,                                                       \
        .threadtype = 0,                                                                          \
        .threadpriority = threadprio                                                              \
    };                                                                                            \
    struct embios_syscall_table* __embios_syscall;                                                \
    const uint32_t __embios_required_version = EMBIOS_API_VERSION;                                \
    const char __embios_incompatible_api_str[] = "Incompatible API version!\nGot %d, need %d";
