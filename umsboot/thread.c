//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emBIOS.
//
//    emBIOS is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emBIOS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include "global.h"
#include "thread.h"
#include "timer.h"
#include "panic.h"
#include "util.h"
#ifdef HAVE_STORAGE
#include "dir.h"
#include "file.h"
#endif
#ifdef HAVE_BUTTON
#include "button.h"
#endif


struct scheduler_thread scheduler_threads[MAX_THREADS] IBSS_ATTR;
struct scheduler_thread* current_thread IBSS_ATTR;
uint32_t last_tick IBSS_ATTR;
bool scheduler_frozen IBSS_ATTR;
extern struct wakeup dbgwakeup;


void mutex_init(struct mutex* obj)
{
    memset(obj, 0, sizeof(struct mutex));
}

void mutex_add_to_queue(struct mutex* obj, struct scheduler_thread* thread)
{
    struct scheduler_thread* t;
    if (!obj->waiters || obj->waiters->priority <= thread->priority)
    {
        thread->queue_next = obj->waiters;
        obj->waiters = thread;
    }
    else
    {
        t = obj->waiters;
        while (t->queue_next && t->queue_next->priority > thread->priority)
            t = t->queue_next;
        thread->queue_next = t->queue_next;
        t->queue_next = thread;
    }
}

void mutex_remove_from_queue(struct mutex* obj, struct scheduler_thread* thread)
{
    struct scheduler_thread* t;
    if (!obj->waiters) return;
    if (obj->waiters == thread) obj->waiters = thread->queue_next;
    else
    {
        t = obj->waiters;
        while (t->queue_next)
        {
            if (t->queue_next == thread) t->queue_next = thread->queue_next;
            t = t->queue_next;
        }
    }
}

int mutex_lock(struct mutex* obj, int timeout)
{
    int ret = THREAD_OK;
    struct scheduler_thread* thread;
    uint32_t mode = enter_critical_section();

    if (!obj->count)
    {
        obj->count = 1;
        obj->owner = current_thread;
    }
    else if (obj->owner == current_thread) obj->count++;
    else
    {
        if (timeout)
        {
            current_thread->state = THREAD_BLOCKED;
            current_thread->block_type = THREAD_BLOCK_MUTEX;
            current_thread->blocked_by = obj;
            current_thread->timeout = timeout;
            current_thread->blocked_since = USEC_TIMER;
            mutex_add_to_queue(obj, current_thread);
            leave_critical_section(mode);
            context_switch();
            if (obj->owner != current_thread) return THREAD_TIMEOUT;
            return THREAD_OK;
        }
        else ret = THREAD_TIMEOUT;
    }

    leave_critical_section(mode);
    return ret;
}

int mutex_unlock(struct mutex* obj)
{
    int ret = THREAD_OK;
    uint32_t mode = enter_critical_section();

    if (!obj->count)
    {
        leave_critical_section(mode);
        panicf(PANIC_KILLTHREAD, "Trying to unlock non-owned mutex! (%08X)", obj);
    }

    if (obj->owner != current_thread)
    {
        leave_critical_section(mode);
        panicf(PANIC_KILLTHREAD, "Trying to unlock mutex owned by different thread! (%08X)", obj);
    }

    if (--(obj->count)) ret = obj->count;
    else if (obj->waiters)
    {
        obj->count = 1;
        obj->owner = obj->waiters;
        obj->waiters->state = THREAD_READY;
        obj->waiters->block_type = THREAD_NOT_BLOCKED;
        obj->waiters->blocked_by = NULL;
        obj->waiters->timeout = 0;
        obj->waiters = obj->waiters->queue_next;
    }

    leave_critical_section(mode);
    return ret;
}

void wakeup_init(struct wakeup* obj)
{
    memset(obj, 0, sizeof(struct wakeup));
}

int wakeup_wait(struct wakeup* obj, int timeout)
{
    int ret = THREAD_OK;
    uint32_t mode = enter_critical_section();

    if (obj->waiter)
    {
        leave_critical_section(mode);
        panicf(PANIC_KILLTHREAD, "Multiple threads waiting single wakeup! (%08X)", obj);
    }

    if (obj->signalled) obj->signalled = false;
    else
    {
        if (timeout)
        {
            current_thread->state = THREAD_BLOCKED;
            current_thread->block_type = THREAD_BLOCK_WAKEUP;
            current_thread->blocked_by = obj;
            current_thread->timeout = timeout;
            current_thread->blocked_since = USEC_TIMER;
            obj->waiter = current_thread;
            leave_critical_section(mode);
            context_switch();
            obj->waiter = NULL;
            if (!obj->signalled) return THREAD_TIMEOUT;
            obj->signalled = false;
            return THREAD_OK;
        }
        else ret = THREAD_TIMEOUT;
    }

    leave_critical_section(mode);
    return ret;
}

int wakeup_signal(struct wakeup* obj)
{
    int ret = THREAD_OK;
    uint32_t mode = enter_critical_section();

    obj->signalled = true;
    if (obj->waiter)
    {
        obj->waiter->state = THREAD_READY;
        obj->waiter->block_type = THREAD_NOT_BLOCKED;
        obj->waiter->blocked_by = NULL;
        obj->waiter->timeout = 0;
        ret = THREAD_FOUND;
    }

    leave_critical_section(mode);
    return ret;
}

void sleep(int usecs)
{
    if (usecs)
    {
        uint32_t mode = enter_critical_section();
        current_thread->state = THREAD_BLOCKED;
        current_thread->block_type = THREAD_BLOCK_SLEEP;
        current_thread->timeout = usecs;
        current_thread->blocked_since = USEC_TIMER;
        leave_critical_section(mode);
    }
    context_switch();
}

void scheduler_init(void)
{
    memset(scheduler_threads, 0, sizeof(scheduler_threads));
    scheduler_frozen = false;
    last_tick = USEC_TIMER;
    current_thread = scheduler_threads;
    current_thread->state = THREAD_RUNNING;
    current_thread->startusec = last_tick;
    current_thread->name = "idle thread";
    current_thread->stack = (uint32_t*)-1;
    setup_tick();
}

bool scheduler_freeze(bool value)
{
    bool old = scheduler_frozen;
    scheduler_frozen = value;
    return old;
}

void scheduler_switch(int thread)
{
    int i;
    uint32_t score, best;
    uint32_t usec = USEC_TIMER;
    if (current_thread->state == THREAD_RUNNING) current_thread->state = THREAD_READY;
    current_thread->cputime_total += usec - current_thread->startusec;
    current_thread->cputime_current += usec - current_thread->startusec;
    if ((int)current_thread->stack != -1 && *current_thread->stack != 0xaffebeaf)
    {
        for (i = 0; i < MAX_THREADS; i++)
            if (scheduler_threads[i].type == USER_THREAD)
                scheduler_threads[i].state = THREAD_SUSPENDED;
        current_thread->state = THREAD_DEFUNCT;
        current_thread->block_type = THREAD_DEFUNCT_STKOV;
        wakeup_signal(&dbgwakeup);
    }

    if (usec - last_tick > SCHEDULER_TICK)
    {
        last_tick = usec;
        for (i = 0; i < MAX_THREADS; i++)
        {
            scheduler_threads[i].cpuload = 255 * scheduler_threads[i].cputime_current / SCHEDULER_TICK;
            scheduler_threads[i].cputime_current = 0;
        }
    }

    if (scheduler_frozen) thread = 0;
    else
    {
        for (i = 0; i < MAX_THREADS; i++)
            if (scheduler_threads[i].state == THREAD_BLOCKED
             && scheduler_threads[i].timeout != -1
             && TIME_AFTER(usec, scheduler_threads[i].blocked_since
                               + scheduler_threads[i].timeout))
            {
                if (scheduler_threads[i].block_type == THREAD_BLOCK_MUTEX)
                    mutex_remove_from_queue((struct mutex*)scheduler_threads[i].blocked_by,
                                            &scheduler_threads[i]);
                scheduler_threads[i].state = THREAD_READY;
                scheduler_threads[i].block_type = THREAD_NOT_BLOCKED;
                scheduler_threads[i].blocked_by = NULL;
                scheduler_threads[i].timeout = 0;
            }

        if (thread >= 0 && thread < MAX_THREADS && scheduler_threads[thread].state == THREAD_READY)
            current_thread = &scheduler_threads[thread];
        else
        {
            thread = 0;
            best = 0xffffffff;
            for (i = 0; i < MAX_THREADS; i++)
                if (scheduler_threads[i].state == THREAD_READY && scheduler_threads[i].priority)
                {
                    score = scheduler_threads[i].cputime_current / scheduler_threads[i].priority;
                    if (score < best)
                    {
                        best = score;
                        thread = i;
                    }
                }
        }
    }

    current_thread = &scheduler_threads[thread];
    current_thread->state = THREAD_RUNNING;
    current_thread->startusec = USEC_TIMER;
}

int thread_create(const char* name, const void* code, void* stack,
                  int stacksize, enum thread_type type, int priority, bool run)
{
    int ret = NO_MORE_THREADS;
    int i;

    for (i = 0; i < stacksize >> 2; i ++) ((uint32_t*)stack)[i] = 0xaffebeaf;

    uint32_t mode = enter_critical_section();

    for (i = 0; i < MAX_THREADS; i++)
        if (scheduler_threads[i].state == THREAD_FREE)
        {
            ret = i;
            memset(&scheduler_threads[i], 0, sizeof(struct scheduler_thread));
            scheduler_threads[i].state = run ? THREAD_READY : THREAD_SUSPENDED;
            scheduler_threads[i].type = type;
            scheduler_threads[i].name = name;
            scheduler_threads[i].priority = priority;
            scheduler_threads[i].cpsr = 0x1f;
            scheduler_threads[i].regs[15] = (uint32_t)code;
            scheduler_threads[i].regs[14] = (uint32_t)thread_exit;
            scheduler_threads[i].regs[13] = (uint32_t)stack + stacksize;
            scheduler_threads[i].stack = stack;
            break;
        }

    leave_critical_section(mode);
    return ret;
}

int thread_suspend(int thread)
{
    int ret = THREAD_OK;
    struct scheduler_thread* t = &scheduler_threads[thread];
    bool needsswitch = false;
    uint32_t mode = enter_critical_section();

    if (thread == -1) t = current_thread;
    else if (thread < 0 || thread >= MAX_THREADS) ret = INVALID_THREAD;
    else if (t->state == THREAD_FREE) ret = INVALID_THREAD;
    else if (t->state == THREAD_SUSPENDED) ret = ALREADY_SUSPENDED;
    if (ret == THREAD_OK)
    {
        if (t->state == THREAD_RUNNING) needsswitch = true;
        else if (t->state == THREAD_BLOCKED)
        {
            if (t->block_type == THREAD_BLOCK_SLEEP)
            {
                if (t->timeout != -1) t->timeout -= USEC_TIMER - t->blocked_since;
            }
            else if (t->block_type == THREAD_BLOCK_MUTEX)
            {
                mutex_remove_from_queue((struct mutex*)t->blocked_by, t);
                if (t->timeout != -1) t->timeout -= USEC_TIMER - t->blocked_since;
            }
            else if (t->block_type == THREAD_BLOCK_WAKEUP)
            {
                if (t->timeout != -1) t->timeout -= USEC_TIMER - t->blocked_since;
            }
        }
        t->state = THREAD_SUSPENDED;
    }

    leave_critical_section(mode);

    if (needsswitch) context_switch();

    return ret;
}

int thread_resume(int thread)
{
    int ret = THREAD_OK;
    struct scheduler_thread* t = &scheduler_threads[thread];
    bool needsswitch = false;
    uint32_t mode = enter_critical_section();

    if (thread == -1) t = current_thread;
    else if (thread < 0 || thread >= MAX_THREADS) ret = INVALID_THREAD;
    else if (t->state == THREAD_FREE) ret = INVALID_THREAD;
    else if (t->state != THREAD_SUSPENDED) ret = ALREADY_RESUMED;
    if (ret == THREAD_OK)
    {
        if (t->block_type == THREAD_BLOCK_SLEEP)
            t->blocked_since = USEC_TIMER;
        else if (t->block_type == THREAD_BLOCK_MUTEX)
        {
            mutex_add_to_queue((struct mutex*)t->blocked_by, t);
            t->blocked_since = USEC_TIMER;
            t->state = THREAD_BLOCKED;
        }
        else if (t->block_type == THREAD_BLOCK_WAKEUP)
        {
            t->blocked_since = USEC_TIMER;
            t->state = THREAD_BLOCKED;
        }
        else t->state = THREAD_READY;
    }

    leave_critical_section(mode);
    return ret;
}

int thread_terminate(int thread)
{
    int ret = THREAD_OK;
    struct scheduler_thread* t = &scheduler_threads[thread];
    bool needsswitch = false;
    uint32_t mode = enter_critical_section();

    if (thread == -1) t = current_thread;
    else if (thread < 0 || thread >= MAX_THREADS) ret = INVALID_THREAD;
    else if (t->state == THREAD_FREE) ret = INVALID_THREAD;
    if (ret == THREAD_OK)
    {
        if (t->state == THREAD_RUNNING) needsswitch = true;
        else if (t->state == THREAD_BLOCKED)
        {
            if (t->block_type == THREAD_BLOCK_MUTEX)
                mutex_remove_from_queue((struct mutex*)t->blocked_by, t);
            else if (t->block_type == THREAD_BLOCK_WAKEUP)
                ((struct wakeup*)t->blocked_by)->waiter = NULL;
        }
        t->state = THREAD_FREE;
#ifdef HAVE_STORAGE
        close_all_of_process(t);
        closedir_all_of_process(t);
#endif
#ifdef HAVE_BUTTON
        button_unregister_all_of_thread(t);
#endif
    }

    leave_critical_section(mode);

    if (needsswitch) context_switch();

    return ret;
}

void thread_exit()
{
    thread_terminate(-1);
}

int* __errno()
{
    return &current_thread->err_no;
}
