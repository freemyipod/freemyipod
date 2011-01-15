#include "syscallwrappers.h"


#define EMCORE_APP_HEADER(threadnamestr, stacksizebytes, mainfunc, threadprio)                    \
    extern char __bss_start;                                                                      \
    extern char __bss_end;                                                                        \
    void __emcore_init()                                                                          \
    {                                                                                             \
        asm volatile("swi\t2\n\tldr\tr3, =__emcore_required_version\nldr\tr3, [r3]\n\t"           \
                     "ldr\tr2, [r0]\n\tcmp\tr3, r2\n\tldrls\tr1, [r0,#4]\n\tcmpls\tr1, r3\n\t"    \
                     "movhi\tr0, #0\n\tldrhi\tr1, =__emcore_incompatible_api_str\n\t"             \
                     "swihi\t1\n\tldr\tr1, =__emcore_syscall\n\tstr\tr0, [r1]\n\t"                \
                 ::: "r0", "r1", "r2", "r3", "r12", "lr", "cc", "memory");                        \
        memset(&__bss_start, 0, (&__bss_end) - (&__bss_start));                                   \
        mainfunc();                                                                               \
    }                                                                                             \
    uint32_t __emcore_thread_stack[stacksizebytes >> 2] __attribute__((section(".stack")));       \
    const char __emcore_thread_name[] = threadnamestr;                                            \
    struct emcore_app_header                                                                      \
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
    } __emcore_executable_hdr __attribute__((section(".execheader"))) =                           \
    {                                                                                             \
        .signature = "emBIexec",                                                                  \
        .version = 0,                                                                             \
        .baseaddr = &__emcore_executable_hdr,                                                     \
        .size = -1,                                                                               \
        .crc32 = 0,                                                                               \
        .stackaddr = __emcore_thread_stack,                                                       \
        .stacksize = stacksizebytes,                                                              \
        .entrypoint = __emcore_init,                                                              \
        .threadname = __emcore_thread_name,                                                       \
        .threadtype = 0,                                                                          \
        .threadpriority = threadprio                                                              \
    };                                                                                            \
    struct emcore_syscall_table* __emcore_syscall __attribute__((section(".stack")));             \
    const uint32_t __emcore_required_version = EMCORE_API_VERSION;                                \
    const char __emcore_incompatible_api_str[] = "Incompatible API version!\nGot %d, need %d";
