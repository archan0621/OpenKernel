#include "process/channel.h"
#include "process/scheduler.h"
#include "arch/x86/idt.h"
#include "mem/kmalloc.h"
#include <stddef.h>

struct channel {
    uint32_t id;
    uint32_t owner_pid;
    channel_message_t queue[CHANNEL_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    task_struct_t* waiting_receiver;
    struct channel* next;
};

static channel_t* channel_list = NULL;
static uint32_t next_channel_id = 1;

static bool channel_interrupts_enabled(void) {
    uint32_t flags;
    __asm__ __volatile__("pushf; pop %0" : "=r"(flags));
    return (flags & 0x200) != 0;
}

static void channel_restore_interrupts(bool enabled) {
    if (enabled) {
        idt_enable_interrupts();
    }
}

static void channel_wait_until_running(task_struct_t* task) {
    if (!task || task->pid == 0) {
        return;
    }

    for (;;) {
        __asm__ __volatile__("sti; hlt");

        if (scheduler_get_current_task() == task && task->state == TASK_RUNNING) {
            return;
        }
    }
}

static bool channel_enqueue(channel_t* channel, const channel_message_t* message) {
    if (channel->count >= CHANNEL_QUEUE_CAPACITY) {
        return false;
    }

    channel->queue[channel->tail] = *message;
    channel->tail = (channel->tail + 1) % CHANNEL_QUEUE_CAPACITY;
    channel->count++;
    return true;
}

static bool channel_dequeue(channel_t* channel, channel_message_t* out_message) {
    if (channel->count == 0) {
        return false;
    }

    *out_message = channel->queue[channel->head];
    channel->head = (channel->head + 1) % CHANNEL_QUEUE_CAPACITY;
    channel->count--;
    return true;
}

void channel_init(void) {
    channel_list = NULL;
    next_channel_id = 1;
}

channel_t* channel_create(void) {
    channel_t* channel = (channel_t*)kmalloc(sizeof(channel_t));
    if (!channel) {
        return NULL;
    }

    task_struct_t* owner = scheduler_get_current_task();

    channel->owner_pid = owner ? owner->pid : 0;
    channel->head = 0;
    channel->tail = 0;
    channel->count = 0;
    channel->waiting_receiver = NULL;

    bool interrupts_enabled = channel_interrupts_enabled();
    idt_disable_interrupts();
    channel->id = next_channel_id++;
    channel->next = channel_list;
    channel_list = channel;
    channel_restore_interrupts(interrupts_enabled);

    return channel;
}

bool channel_send(channel_t* channel, const channel_message_t* message) {
    if (!channel || !message) {
        return false;
    }

    bool interrupts_enabled = channel_interrupts_enabled();
    idt_disable_interrupts();

    bool sent = channel_enqueue(channel, message);
    task_struct_t* receiver = NULL;

    if (sent && channel->waiting_receiver) {
        receiver = channel->waiting_receiver;
        channel->waiting_receiver = NULL;
    }

    channel_restore_interrupts(interrupts_enabled);

    if (receiver) {
        task_unblock(receiver);
    }

    return sent;
}

bool channel_recv(channel_t* channel, channel_message_t* out_message) {
    if (!channel || !out_message) {
        return false;
    }

    task_struct_t* current = scheduler_get_current_task();
    if (!current || current->pid == 0) {
        return false;
    }

    for (;;) {
        bool interrupts_enabled = channel_interrupts_enabled();
        idt_disable_interrupts();

        if (channel_dequeue(channel, out_message)) {
            channel_restore_interrupts(interrupts_enabled);
            return true;
        }

        if (channel->waiting_receiver && channel->waiting_receiver != current) {
            channel_restore_interrupts(interrupts_enabled);
            task_yield();
            continue;
        }

        channel->waiting_receiver = current;
        scheduler_block_current_task();

        channel_wait_until_running(current);
    }
}

uint32_t channel_get_id(const channel_t* channel) {
    return channel ? channel->id : 0;
}

uint32_t channel_get_owner_pid(const channel_t* channel) {
    return channel ? channel->owner_pid : 0;
}

uint32_t channel_get_count(const channel_t* channel) {
    return channel ? channel->count : 0;
}

task_struct_t* channel_get_waiting_receiver(const channel_t* channel) {
    return channel ? channel->waiting_receiver : NULL;
}
