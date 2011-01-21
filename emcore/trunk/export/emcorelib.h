#include "syscallwrappers.h"


#define EMCORE_LIB_HEADER(libidentifier, libversion, initfunc, shutdownfunc, apipointer)          \
    struct emcore_syscall_table* __emcore_syscall;                                                \
    int __emcore_lib_init()                                                                       \
    {                                                                                             \
        asm volatile("swi\t2\n\tldr\tr1, =__emcore_syscall\n\tstr\tr0, [r1]\n\t"                  \
                 ::: "r0", "r1", "r2", "r3", "r12", "lr", "cc", "memory");                        \
        if (__embios_syscall->table_version < EMCORE_API_VERSION                                  \
         || __embios_syscall->table_minversion > EMCORE_API_VERSION)                              \
             return 0x80000000;                                                                   \
        if (initfunc) return initfunc();                                                          \
    }                                                                                             \
    struct emcorelib_header __attribute__((section(".emcoreentrypoint"))) __emcore_entrypoint()   \
    {                                                                                             \
        .headerversion = EMCORELIB_HEADER_VERSION,                                                \
        .identifier = libidentifier,                                                              \
        .version = libversion,                                                                    \
        .initfunc = __emcore_lib_init,                                                            \
        .shutdownfunc = shutdownfunc,                                                             \
        .api = apipointer                                                                         \
    };
