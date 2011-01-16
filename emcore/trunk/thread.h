//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emCORE.
//
//    emCORE is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emCORE is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emCORE.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __THREAD_H__
#define __THREAD_H__


#include "global.h"
#include "contextswitch.h"


#define TIMEOUT_NONE 0
#define TIMEOUT_BLOCK -1

#define THREAD_FOUND 1
#define THREAD_OK 0
#define THREAD_TIMEOUT -1
#define ALREADY_SUSPENDED -4
#define ALREADY_RESUMED -5


enum thread_state
{
    THREAD_FREE = 0,
    THREAD_SUSPENDED = 1,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_DEFUNCT,
    THREAD_DEFUNCT_ACK
};

enum thread_block
{
    THREAD_NOT_BLOCKED = 0,
    THREAD_BLOCK_SLEEP,
    THREAD_BLOCK_MUTEX,
    THREAD_BLOCK_WAKEUP,
    THREAD_DEFUNCT_STKOV,
    THREAD_DEFUNCT_PANIC
};

enum thread_type
{
    USER_THREAD = 0,
    OS_THREAD,
    CORE_THREAD
};


#define SCHEDULER_THREAD_INFO_VERSION 2

struct scheduler_thread
{
    uint32_t regs[16];
    uint32_t cpsr;
    uint32_t state;
    const char* name;
    uint32_t cputime_current;
    uint64_t cputime_total;
    uint32_t startusec;
    struct scheduler_thread* thread_next;
    struct scheduler_thread* queue_next;
    uint32_t timeout;
    uint32_t blocked_since;
    void* blocked_by;
    uint32_t* stack;
    int err_no;
    enum thread_block block_type;
    enum thread_type type;
    uint8_t priority;
    uint8_t cpuload;
};

struct mutex
{
    struct scheduler_thread* owner;
    struct scheduler_thread* waiters;
    int count;
};

struct wakeup
{
    struct scheduler_thread* waiter;
    bool signalled;
};


extern struct scheduler_thread* head_thread IBSS_ATTR;
extern struct scheduler_thread* current_thread IBSS_ATTR;


void scheduler_init() INITCODE_ATTR;
void scheduler_pause_accounting() ICODE_ATTR;
void scheduler_resume_accounting() ICODE_ATTR;
void scheduler_switch(struct scheduler_thread* thread) ICODE_ATTR;
bool scheduler_freeze(bool value);
struct scheduler_thread* thread_create(struct scheduler_thread* thread, const char* name,
                                       const void* code, void* stack, int stacksize,
                                       enum thread_type type, int priority, bool run);
int thread_suspend(struct scheduler_thread* thread);
int thread_resume(struct scheduler_thread* thread);
int thread_terminate(struct scheduler_thread* thread);
int thread_killlevel(enum thread_type type, bool killself);
enum thread_state thread_get_state(struct scheduler_thread* thread);
void thread_exit();
void mutex_init(struct mutex* obj) ICODE_ATTR;
int mutex_lock(struct mutex* obj, int timeout) ICODE_ATTR;
int mutex_unlock(struct mutex* obj) ICODE_ATTR;
void wakeup_init(struct wakeup* obj) ICODE_ATTR;
int wakeup_wait(struct wakeup* obj, int timeout) ICODE_ATTR;
int wakeup_signal(struct wakeup* obj) ICODE_ATTR;
void sleep(int usecs) ICODE_ATTR;

static inline void yield()
{
    context_switch();
}

#endif
