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


#include "global.h"
#include "thread.h"
#include "timer.h"
#include "panic.h"
#include "util.h"
#include "malloc.h"
#include "library.h"
#ifdef HAVE_STORAGE
#include "dir.h"
#include "file.h"
#endif
#ifdef HAVE_BUTTON
#include "button.h"
#endif


struct scheduler_thread* head_thread IBSS_ATTR;
struct scheduler_thread* current_thread IBSS_ATTR;
struct scheduler_thread idle_thread IBSS_ATTR;
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
            yield();
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
        panicf(PANIC_KILLTHREAD, "Multiple threads waiting for single wakeup! (%08X)", obj);
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
            yield();
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
        if (current_thread == &idle_thread)
            scheduler_switch(obj->waiter, NULL);
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
    yield();
}

void scheduler_init(void)
{
    last_tick = USEC_TIMER;
    scheduler_frozen = false;
    head_thread = &idle_thread;
    current_thread = &idle_thread;
    memset(&idle_thread, 0, sizeof(idle_thread));
    idle_thread.state = THREAD_RUNNING;
    idle_thread.startusec = last_tick;
    idle_thread.type = CORE_THREAD;
    idle_thread.name = "idle thread";
    idle_thread.stack = (uint32_t*)-1;
}

bool scheduler_freeze(bool value)
{
    bool old = scheduler_frozen;
    scheduler_frozen = value;
    return old;
}

void scheduler_pause_accounting()
{
    uint32_t usec = USEC_TIMER;
    current_thread->cputime_total += usec - current_thread->startusec;
    current_thread->cputime_current += usec - current_thread->startusec;
}

void scheduler_resume_accounting()
{
    current_thread->startusec = USEC_TIMER;
}

void scheduler_switch(struct scheduler_thread* thread, struct scheduler_thread* block)
{
    struct scheduler_thread* t;
    uint32_t score, best;
    uint32_t usec = USEC_TIMER;
    if (current_thread->state == THREAD_RUNNING) current_thread->state = THREAD_READY;
    if ((int)current_thread->stack != -1 && *current_thread->stack != 0xaffebeaf)
    {
        for (t = head_thread; t; t = t->thread_next)
            if (t->type == USER_THREAD)
                t->state = THREAD_SUSPENDED;
        current_thread->state = THREAD_DEFUNCT;
        current_thread->block_type = THREAD_DEFUNCT_STKOV;
        wakeup_signal(&dbgwakeup);
    }

    timer_kill_wakeup();

    if (usec - last_tick > SCHEDULER_TICK)
    {
        uint32_t diff = usec - last_tick;
        last_tick = usec;
        for (t = head_thread; t; t = t->thread_next)
        {
            t->cpuload = 255 * t->cputime_current / diff;
            t->cputime_current = 0;
        }
    }

    uint32_t next_unblock = 0xffffffff;
    if (scheduler_frozen) thread = &idle_thread;
    else
    {
        for (t = head_thread; t; t = t->thread_next)
        {
            if (t->state == THREAD_BLOCKED && t->timeout != -1
             && TIME_AFTER(usec, t->blocked_since + t->timeout))
            {
                if (t->block_type == THREAD_BLOCK_MUTEX)
                    mutex_remove_from_queue((struct mutex*)t->blocked_by, t);
                t->state = THREAD_READY;
                t->block_type = THREAD_NOT_BLOCKED;
                t->blocked_by = NULL;
                t->timeout = 0;
            }
            else if (t->state == THREAD_BLOCKED && t->timeout != -1)
            {
                uint32_t left = t->blocked_since + t->timeout - usec;
                if (left < next_unblock) next_unblock = left;
            }
        }

        if (!thread || thread->state != THREAD_READY)
        {
            thread = &idle_thread;
            best = 0xffffffff;
            for (t = head_thread; t; t = t->thread_next)
                if (t->state == THREAD_READY && t->priority)
                {
                    if (t == block) score = 0xfffffffe;
                    else score = t->cputime_current / t->priority;
                    if (score < best)
                    {
                        best = score;
                        thread = t;
                    }
                }
        }

        if (thread == &idle_thread) timer_schedule_wakeup(next_unblock);
        else timer_schedule_wakeup(SYSTEM_TICK);
    }

    current_thread = thread;
    current_thread->state = THREAD_RUNNING;
}

struct scheduler_thread* thread_create(struct scheduler_thread* thread, const char* name,
                                       const void* code, void* stack, int stacksize,
                                       enum thread_type type, int priority, bool run)
{
    bool stack_alloced = false;
    bool thread_alloced = false;
    if (!stack)
    {
        stack = malloc(stacksize);
        stack_alloced = true;
    }
    if (!stack) return NULL;
    if (!thread)
    {
        thread = (struct scheduler_thread*)malloc(sizeof(struct scheduler_thread));
        thread_alloced = true;
    }
    if (!thread)
    {
        if (stack_alloced) free(stack);
        return NULL;
    }
    if (thread_alloced) reownalloc(thread, thread);
    if (stack_alloced) reownalloc(stack, thread);

    int i;
    for (i = 0; i < stacksize >> 2; i ++) ((uint32_t*)stack)[i] = 0xaffebeaf;

    memset(thread, 0, sizeof(struct scheduler_thread));
    thread->state = run ? THREAD_READY : THREAD_SUSPENDED;
    thread->type = type;
    thread->name = name;
    thread->priority = priority;
    thread->cpsr = 0x1f;
    thread->regs[15] = (uint32_t)code;
    thread->regs[14] = (uint32_t)thread_exit;
    thread->regs[13] = (uint32_t)stack + stacksize;
    thread->stack = stack;

    uint32_t mode = enter_critical_section();
    thread->thread_next = head_thread->thread_next;
    head_thread->thread_next = thread;
    leave_critical_section(mode);

    return thread;
}

int thread_suspend(struct scheduler_thread* thread)
{
    int ret = THREAD_OK;
    bool needsswitch = false;
    uint32_t mode = enter_critical_section();

    if (!thread) thread = current_thread;
    if (thread->state == THREAD_SUSPENDED) ret = ALREADY_SUSPENDED;
    if (ret == THREAD_OK)
    {
        if (thread->state == THREAD_RUNNING) needsswitch = true;
        else if (thread->state == THREAD_BLOCKED)
        {
            if (thread->block_type == THREAD_BLOCK_SLEEP)
            {
                if (thread->timeout != -1) thread->timeout -= USEC_TIMER - thread->blocked_since;
            }
            else if (thread->block_type == THREAD_BLOCK_MUTEX)
            {
                mutex_remove_from_queue((struct mutex*)thread->blocked_by, thread);
                if (thread->timeout != -1) thread->timeout -= USEC_TIMER - thread->blocked_since;
            }
            else if (thread->block_type == THREAD_BLOCK_WAKEUP)
            {
                if (thread->timeout != -1) thread->timeout -= USEC_TIMER - thread->blocked_since;
            }
        }
        thread->state = THREAD_SUSPENDED;
    }

    leave_critical_section(mode);

    if (needsswitch) yield();

    return ret;
}

int thread_resume(struct scheduler_thread* thread)
{
    int ret = THREAD_OK;
    bool needsswitch = false;
    uint32_t mode = enter_critical_section();

    if (!thread) thread = current_thread;
    if (thread->state != THREAD_SUSPENDED) ret = ALREADY_RESUMED;
    if (ret == THREAD_OK)
    {
        if (thread->block_type == THREAD_BLOCK_SLEEP)
            thread->blocked_since = USEC_TIMER;
        else if (thread->block_type == THREAD_BLOCK_MUTEX)
        {
            mutex_add_to_queue((struct mutex*)thread->blocked_by, thread);
            thread->blocked_since = USEC_TIMER;
            thread->state = THREAD_BLOCKED;
        }
        else if (thread->block_type == THREAD_BLOCK_WAKEUP)
        {
            thread->blocked_since = USEC_TIMER;
            thread->state = THREAD_BLOCKED;
        }
        else thread->state = THREAD_READY;
    }

    leave_critical_section(mode);
    return ret;
}

void thread_set_name(struct scheduler_thread* thread, char* name)
{
    uint32_t mode = enter_critical_section();
    if (!thread) thread = current_thread;
    thread->name = name;
    leave_critical_section(mode);
}

void thread_set_priority(struct scheduler_thread* thread, int priority)
{
    uint32_t mode = enter_critical_section();
    if (!thread) thread = current_thread;
    thread->priority = priority;
    leave_critical_section(mode);
}

int thread_terminate_internal(struct scheduler_thread* thread, uint32_t mode)
{
    struct scheduler_thread* t;
    bool needsswitch = false;

    if (!thread) thread = current_thread;
    if (thread->state == THREAD_RUNNING) needsswitch = true;
    else
    {
        if (thread->state == THREAD_BLOCKED)
        {
            if (thread->block_type == THREAD_BLOCK_MUTEX)
                mutex_remove_from_queue((struct mutex*)t->blocked_by, thread);
            else if (thread->block_type == THREAD_BLOCK_WAKEUP)
                ((struct wakeup*)thread->blocked_by)->waiter = NULL;
        }
        thread->state = THREAD_SUSPENDED;
    }

    leave_critical_section(mode);

    library_release_all_of_thread(thread);
#ifdef HAVE_STORAGE
    close_all_of_process(thread);
    closedir_all_of_process(thread);
#endif
#ifdef HAVE_BUTTON
    button_unregister_all_of_thread(thread);
#endif
    free_all_of_thread(thread);

    mode = enter_critical_section();
    for (t = head_thread; t && t->thread_next != thread; t = t->thread_next);
    if (t) t->thread_next = thread->thread_next;
    leave_critical_section(mode);

    if (needsswitch) yield();

    return THREAD_OK;
}

int thread_terminate(struct scheduler_thread* thread)
{
    uint32_t mode = enter_critical_section();
    return thread_terminate_internal(thread, mode);
}

int thread_killlevel(enum thread_type type, bool killself)
{
    struct scheduler_thread* t;
    int count = 0;
    while (true)
    {
        bool found = false;
        uint32_t mode = enter_critical_section();
        for (t = head_thread; t; t = t->thread_next)
            if (t->type <= type && current_thread != t)
            {
                thread_terminate_internal(t, mode);
                found = true;
                count++;
                break;
            }
        if (found) continue;
        leave_critical_section(mode);
        break;
    }
    if (killself) thread_exit();
    return count;
}

enum thread_state thread_get_state(struct scheduler_thread* thread)
{
    return thread->state;
}

void thread_exit()
{
    thread_terminate(NULL);
}

int* __errno()
{
    return &current_thread->err_no;
}
