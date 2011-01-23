#include "syscallwrappers.h"


#define EMCORE_LIB_HEADER(lib_identifier, lib_version, lib_min_version, init_func, shutdown_func, \
                          api_pointer)                                                            \
    struct emcore_syscall_table* __emcore_syscall;                                                \
    int __emcore_lib_init()                                                                       \
    {                                                                                             \
        asm volatile("swi\t2\n\tldr\tr1, =__emcore_syscall\n\tstr\tr0, [r1]\n\t"                  \
                 ::: "r0", "r1", "r2", "r3", "r12", "lr", "cc", "memory");                        \
        if (__emcore_syscall->table_version < EMCORE_API_VERSION                                  \
         || __emcore_syscall->table_minversion > EMCORE_API_VERSION)                              \
             return 0x80000000;                                                                   \
    }                                                                                             \
    struct emcorelib_header __emcore_lib_header __attribute__((section(".emcoreentrypoint"))) =   \
    {                                                                                             \
        .headerversion = EMCORELIB_HEADER_VERSION,                                                \
        .identifier = lib_identifier,                                                             \
        .version = lib_version,                                                                   \
        .minversion = lib_min_version,                                                            \
        .setupfunc = __emcore_lib_init,                                                           \
        .initfunc = init_func,                                                                    \
        .shutdownfunc = shutdown_func,                                                            \
        .api = &api_pointer                                                                       \
    };


extern struct emcore_syscall_table* __emcore_syscall;
