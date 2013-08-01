#include "global.h"
#include "sys/util.h"

__attribute__((weak,noreturn)) void fault_handler()
{
    hang();
}

__attribute__((weak,alias("fault_handler"))) void _undef_instr_handler();
__attribute__((weak,alias("fault_handler"))) void _syscall_handler();
__attribute__((weak,alias("fault_handler"))) void _prefetch_abort_handler();
__attribute__((weak,alias("fault_handler"))) void _data_abort_handler();
__attribute__((weak,alias("fault_handler"))) void _reserved_handler();
__attribute__((weak,alias("fault_handler"))) void _fiq_handler();
__attribute__((weak,alias("fault_handler"))) void _irq_handler();
__attribute__((weak,alias("fault_handler"))) void __div0();
