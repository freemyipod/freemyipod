#include "syscallwrappers.h"


#define EMCORE_APP_HEADER(threadnamestr, mainfunc, threadprio)                                    \
    void __attribute__((section(".emcoreentrypoint"))) __emcore_entrypoint()                      \
    {                                                                                             \
        asm volatile("swi\t2\n\tldr\tr3, =__emcore_required_version\nldr\tr3, [r3]\n\t"           \
                     "ldr\tr2, [r0]\n\tcmp\tr3, r2\n\tldrls\tr1, [r0,#4]\n\tcmpls\tr1, r3\n\t"    \
                     "movhi\tr0, #0\n\tldrhi\tr1, =__emcore_incompatible_api_str\n\t"             \
                     "swihi\t1\n\tldr\tr1, =__emcore_syscall\n\tstr\tr0, [r1]\n\t"                \
                 ::: "r0", "r1", "r2", "r3", "r12", "lr", "cc", "memory");                        \
        thread_set_name(NULL, threadnamestr);                                                     \
        thread_set_priority(NULL, threadprio);                                                    \
        mainfunc();                                                                               \
    }                                                                                             \
    struct emcore_syscall_table* __emcore_syscall;                                                \
    const uint32_t __emcore_required_version = EMCORE_API_VERSION;                                \
    const char __emcore_incompatible_api_str[] = "Incompatible API version!\nGot %d, need %d";
