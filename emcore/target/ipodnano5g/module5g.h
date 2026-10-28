/* GPL-2.0-or-later. SDRAM module ABI; bump the version for incompatible changes.
 * Modules are trusted native code, installed once per boot, before NAND use.
 * ABI 2 only appends the peripheral lease services to the table: the kernel
 * still runs ABI 1 modules, and a kernel without leases rejects ABI 2 modules
 * at activation, before any module code runs. */
#ifndef MODULE5G_H
#define MODULE5G_H
#define N5G_MODULE_MAGIC 0x4d47354e
#define N5G_MODULE_ABI 2
#define N5G_MODULE_ABI_MIN 1
#define N5G_MODULE_INFO 132
#define N5G_MODULE_RESERVE 133
#define N5G_MODULE_ACTIVATE 134
#define N5G_MODULE_MAX (2 * 1024 * 1024)

/* Stable import indices. The ARM trampolines preserve all arguments and LR. */
#define N5G_MODULE_IMPORTS(X) \
    X(0, memalign) X(1, reownalloc) X(2, memset) X(3, memcpy) \
    X(4, memmove) X(5, memcmp) X(6, cprintf) X(7, read_usec_timer) \
    X(8, sleep) X(9, thread_create) X(10, wakeup_init) X(11, wakeup_wait) \
    X(12, wakeup_signal) X(13, enter_critical_section) \
    X(14, leave_critical_section) X(15, crc32)

/* Exclusive peripheral leases (ABI 2, lease version 1). The calling thread owns
 * the lease; only that thread can release it. Acquire waits at most timeout_us
 * for emCORE's own users to finish. Release restores the documented emCORE
 * state of the peripheral (I2C: clocks + controller init of both buses; LCD:
 * clocks, controller modes, panel init table, console repaint) before other
 * emCORE users run again. NAND: the module loader already keeps emCORE's NAND
 * harness idle for the whole session; the lease only confirms that. WHEEL
 * (lease version 2): emCORE's poller thread stops sending between two
 * exchanges; release re-runs the controller init and resumes polling).
 * Verify runs emCORE's own driver after release and reports what it saw. */
#define N5G_LEASE_VERSION 2
#define N5G_LEASE_I2C  1u
#define N5G_LEASE_LCD  2u
#define N5G_LEASE_NAND 4u
#define N5G_LEASE_WHEEL 8u  /* lease version 2: the click wheel controller; emCORE's poller pauses */
#define N5G_LEASE_OK       0
#define N5G_LEASE_TIMEOUT  1   /* emCORE still uses it; nothing changed */
#define N5G_LEASE_HELD     2   /* already leased (acquire) / not held (release) */
#define N5G_LEASE_INVALID  3   /* unknown resource, more than one bit */
#define N5G_LEASE_OWNER    4   /* release from a thread that does not own it */
#define N5G_LEASE_VERIFY_WORDS 12

#ifndef ASM_FILE
#include "global.h"
struct n5g_module_header {
    uint32_t magic, abi, kind, text_size, bss_size;
    uint32_t entry, command, heartbeat, reloc_count, reserved;
};
/* current_thread is a pointer to the kernel's scheduler variable. */
struct n5g_module_services {
    uint32_t functions[16];
    struct scheduler_thread **current;
    /* ABI 2 */
    uint32_t lease_version;
    int (*lease_acquire)(uint32_t resource, uint32_t timeout_us);
    int (*lease_release)(uint32_t resource);
    int (*lease_verify)(uint32_t resource, uint32_t *out, uint32_t words);
};
int n5g_lease_acquire(uint32_t resource, uint32_t timeout_us);
int n5g_lease_release(uint32_t resource);
int n5g_lease_verify(uint32_t resource, uint32_t *out, uint32_t words);
uint32_t n5g_lease_held(void);
extern const struct n5g_module_services *n5g_module_services;
extern struct scheduler_thread **n5g_module_current_thread;
int n5g_nand_module_can_load(void);
int n5g_module_blocks_nand(void);
int n5g_module_dispatch(const uint32_t *buf, uint32_t *reply);
int n5g_module_heartbeat(uint32_t *w0, uint32_t *w1);
void n5g_module_command(const uint32_t *buf, uint32_t *reply);
#endif
#endif
