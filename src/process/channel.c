#include "process/channel.h"
#include "process/scheduler.h"
#include "arch/x86/idt.h"
#include "mem/kmalloc.h"
#include <stddef.h>

typedef struct channel_wait_queue {
    task_struct_t* head;
    task_struct_t* tail;
    uint32_t count;
} channel_wait_queue_t;

struct channel {
    uint32_t id;
    uint32_t owner_pid;
    channel_message_t queue[CHANNEL_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    channel_wait_queue_t receiver_wait_queue;
    channel_wait_queue_t sender_wait_queue;
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

static void channel_wait_queue_init(channel_wait_queue_t* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
}

static bool channel_wait_queue_contains(const channel_wait_queue_t* queue, const task_struct_t* task) {
    task_struct_t* current = queue->head;

    while (current) {
        if (current == task) {
            return true;
        }

        current = current->next;
    }

    return false;
}

static void channel_wait_queue_push(channel_wait_queue_t* queue, task_struct_t* task) {
    if (channel_wait_queue_contains(queue, task)) {
        return;
    }

    task->next = NULL;
    task->prev = queue->tail;

    if (queue->tail) {
        queue->tail->next = task;
    } else {
        queue->head = task;
    }

    queue->tail = task;
    queue->count++;
}

static task_struct_t* channel_wait_queue_pop(channel_wait_queue_t* queue) {
    task_struct_t* task = queue->head;
    if (!task) {
        return NULL;
    }

    queue->head = task->next;
    if (queue->head) {
        queue->head->prev = NULL;
    } else {
        queue->tail = NULL;
    }

    task->next = NULL;
    task->prev = NULL;

    if (queue->count > 0) {
        queue->count--;
    }

    return task;
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
    channel_wait_queue_init(&channel->receiver_wait_queue);
    channel_wait_queue_init(&channel->sender_wait_queue);

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

    task_struct_t* current = scheduler_get_current_task();

    for (;;) {
        bool interrupts_enabled = channel_interrupts_enabled();
        idt_disable_interrupts();

        if (channel_enqueue(channel, message)) {
            task_struct_t* receiver = channel_wait_queue_pop(&channel->receiver_wait_queue);

            channel_restore_interrupts(interrupts_enabled);

            if (receiver) {
                task_unblock(receiver);
            }

            return true;
        }

        if (!current || current->pid == 0) {
            channel_restore_interrupts(interrupts_enabled);
            return false;
        }

        channel_wait_queue_push(&channel->sender_wait_queue, current);
        scheduler_block_current_task();

        channel_wait_until_running(current);
    }
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
            task_struct_t* sender = channel_wait_queue_pop(&channel->sender_wait_queue);

            channel_restore_interrupts(interrupts_enabled);

            if (sender) {
                task_unblock(sender);
            }

            return true;
        }

        channel_wait_queue_push(&channel->receiver_wait_queue, current);
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

uint32_t channel_get_waiting_receiver_count(const channel_t* channel) {
    return channel ? channel->receiver_wait_queue.count : 0;
}

task_struct_t* channel_peek_waiting_receiver(const channel_t* channel) {
    return channel ? channel->receiver_wait_queue.head : NULL;
}

uint32_t channel_get_waiting_sender_count(const channel_t* channel) {
    return channel ? channel->sender_wait_queue.count : 0;
}

task_struct_t* channel_peek_waiting_sender(const channel_t* channel) {
    return channel ? channel->sender_wait_queue.head : NULL;
}
