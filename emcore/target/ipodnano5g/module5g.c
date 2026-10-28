/* GPL-2.0-or-later. Upload, validate, relocate, then activate trusted SDRAM code.
 * No hardware access on load. A module cannot be replaced or unloaded because
 * its worker, callback addresses and NAND allocations live until reset. */
#include "global.h"
#include "thread.h"
#include "malloc.h"
#include "console.h"
#include "timer.h"
#include "crc32.h"
#include "mmu.h"
#include "module5g.h"

#include "i2c.h"

#define IMPORT(i, name) [i] = (uint32_t)&name,
static const struct n5g_module_services services = {
    { N5G_MODULE_IMPORTS(IMPORT) }, &current_thread,
    N5G_LEASE_VERSION, n5g_lease_acquire, n5g_lease_release, n5g_lease_verify
};
#undef IMPORT

int i2c_lease_acquire(int timeout);
void i2c_lease_release(void);
int lcd_lease_acquire(int timeout);
void lcd_lease_release(void);
int lcd_lease_verify(uint32_t *out, uint32_t words);
int clickwheel_lease_acquire(int timeout);
void clickwheel_lease_release(void);
int clickwheel_lease_verify(uint32_t *out, uint32_t words);
extern uint32_t i2c_timeouts;

/* Lease bookkeeping (module5g.h). Owners are threads; the lease outlives nothing but the session. */
#define LEASES 4
static struct scheduler_thread *lease_owner[LEASES];

static int lease_index(uint32_t resource)
{
    if (resource == N5G_LEASE_I2C) return 0;
    if (resource == N5G_LEASE_LCD) return 1;
    if (resource == N5G_LEASE_NAND) return 2;
    if (resource == N5G_LEASE_WHEEL) return 3;
    return -1;
}

uint32_t n5g_lease_held(void)
{
    uint32_t i, held = 0;
    for (i = 0; i < LEASES; i++) if (lease_owner[i]) held |= 1u << i;
    return held;
}

int n5g_lease_acquire(uint32_t resource, uint32_t timeout_us)
{
    int i = lease_index(resource), rc;
    int timeout = timeout_us > 0x7fffffff ? 0x7fffffff : (int)timeout_us;
    if (i < 0) return N5G_LEASE_INVALID;
    if (lease_owner[i]) return N5G_LEASE_HELD;
    if (resource == N5G_LEASE_I2C) rc = i2c_lease_acquire(timeout);
    else if (resource == N5G_LEASE_LCD) rc = lcd_lease_acquire(timeout);
    else if (resource == N5G_LEASE_WHEEL) rc = clickwheel_lease_acquire(timeout);
    /* A loaded module blocks every emCORE NAND job for the rest of the session (n5g_module_blocks_nand). */
    else rc = n5g_module_blocks_nand() ? 0 : -1;
    if (rc) return N5G_LEASE_TIMEOUT;
    lease_owner[i] = current_thread;
    return N5G_LEASE_OK;
}

int n5g_lease_release(uint32_t resource)
{
    int i = lease_index(resource);
    if (i < 0) return N5G_LEASE_INVALID;
    if (!lease_owner[i]) return N5G_LEASE_HELD;
    if (lease_owner[i] != current_thread) return N5G_LEASE_OWNER;
    if (resource == N5G_LEASE_I2C) i2c_lease_release();
    else if (resource == N5G_LEASE_LCD) lcd_lease_release();
    else if (resource == N5G_LEASE_WHEEL) clickwheel_lease_release();
    lease_owner[i] = NULL;
    return N5G_LEASE_OK;
}

/* emCORE's own driver after a release. I2C: i2c_timeouts before/after and the PMU registers emCORE reads at
   boot (power_init: 04 05 0a 30 31 33 34; none of them read-to-clear). */
int n5g_lease_verify(uint32_t resource, uint32_t *out, uint32_t words)
{
    static const uint8_t pmu_regs[7] = { 0x04, 0x05, 0x0a, 0x30, 0x31, 0x33, 0x34 };
    uint32_t v[N5G_LEASE_VERIFY_WORDS] = { 0 }, i;
    int i_res = lease_index(resource);
    if (i_res < 0) return N5G_LEASE_INVALID;
    if (lease_owner[i_res]) return N5G_LEASE_HELD;
    if (resource == N5G_LEASE_LCD) return lcd_lease_verify(out, words);
    if (resource == N5G_LEASE_WHEEL) return clickwheel_lease_verify(out, words);
    if (resource == N5G_LEASE_I2C)
    {
        v[0] = i2c_timeouts;
        for (i = 0; i < 7; i++) v[1 + i] = i2c_recvbyte(0, 0xe6, pmu_regs[i]);
        v[8] = i2c_timeouts;
    }
    else v[0] = n5g_module_blocks_nand();
    for (i = 0; i < words && i < N5G_LEASE_VERIFY_WORDS; i++) out[i] = v[i];
    return N5G_LEASE_OK;
}

/* 0 empty, 1 staging (NAND locked), 2 active, 3 failed activation (reset). */
static volatile uint32_t state;
static uint8_t *image;
static uint32_t capacity;
static void (*module_command)(const uint32_t *, uint32_t *);
static void (*module_heartbeat)(uint32_t *, uint32_t *);

int n5g_module_blocks_nand(void) { return state != 0; }
int n5g_module_dispatch(const uint32_t *buf, uint32_t *reply)
{
    if (state != 2) return 0;
    module_command(buf, reply);
    return 1;
}
int n5g_module_heartbeat(uint32_t *w0, uint32_t *w1)
{
    if (state != 2) return 0;
    module_heartbeat(w0, w1);
    return 1;
}

static uint32_t activate(uint32_t size, uint32_t crc)
{
    struct n5g_module_header h;
    uint32_t i, total, mode;
    if (state != 1) return 1;
    if (size < sizeof(h) || size > capacity) return 2;
    if (crc32(image, size) != crc) return 3;
    memcpy(&h, image, sizeof(h));
    if (h.magic != N5G_MODULE_MAGIC || h.abi < N5G_MODULE_ABI_MIN || h.abi > N5G_MODULE_ABI || h.kind != 1 || h.reserved) return 4;
    if (!h.text_size || (h.text_size & 3) || h.text_size > capacity || h.bss_size > capacity - h.text_size) return 5;
    total = h.text_size + h.bss_size;
    if (h.text_size > size - sizeof(h) ||
        h.reloc_count != (size - sizeof(h) - h.text_size) / 4 ||
        (size - sizeof(h) - h.text_size) % 4) return 5;
    if ((h.entry | h.command | h.heartbeat) & 3 || h.entry >= h.text_size ||
        h.command >= h.text_size || h.heartbeat >= h.text_size) return 6;
    uint32_t *relocs = (uint32_t *)(image + sizeof(h) + h.text_size);
    uint8_t *text = image + sizeof(h);
    /* Check every relocation before changing anything, including duplicates. */
    for (i = 0; i < h.reloc_count; i++) {
        uint32_t off = relocs[i];
        if (off & 3 || off > h.text_size - 4 || (i && off <= relocs[i - 1]) ||
            *(uint32_t *)(text + off) > total) return 7;
    }
    state = 3; /* From here onward an error requires a reset. */
    for (i = 0; i < h.reloc_count; i++)
        *(uint32_t *)(text + relocs[i]) += (uint32_t)image;
    memmove(image, text, h.text_size);
    memset(image + h.text_size, 0, h.bss_size);
    clean_dcache();
    invalidate_icache();
    if (((int (*)(const struct n5g_module_services *))(image + h.entry))(&services)) return 8;
    mode = enter_critical_section();
    module_command = (void *)(image + h.command);
    module_heartbeat = (void *)(image + h.heartbeat);
    state = 2;
    leave_critical_section(mode);
    return 0;
}

/* INFO may run in IRQ context. RESERVE / ACTIVATE run on the monitor thread. */
void n5g_module_command(const uint32_t *buf, uint32_t *reply)
{
    uint32_t rc = 0;
    if (buf[0] == N5G_MODULE_RESERVE) {
        uint32_t mode = enter_critical_section();
        if (state || !n5g_nand_module_can_load()) rc = 1;
        else if (!buf[1] || buf[1] > N5G_MODULE_MAX) rc = 2;
        else state = 1; /* Lock the read-only job queue before allocating. */
        leave_critical_section(mode);
        if (!rc) {
            image = memalign(32, buf[1]);
            if (!image) { state = 0; rc = 9; }
            else {
                capacity = buf[1];
                reownalloc(image, KERNEL_OWNER(KERNEL_OWNER_UNKNOWN));
            }
        }
    } else if (buf[0] == N5G_MODULE_ACTIVATE) rc = activate(buf[1], buf[2]);
    reply[0] = 1;
    reply[1] = buf[0] == N5G_MODULE_INFO ? state : rc;
    reply[2] = (uint32_t)image;
    reply[3] = capacity;
}
