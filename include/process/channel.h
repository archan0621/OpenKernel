#pragma once

#include "process/task.h"
#include <stdint.h>
#include <stdbool.h>

#define CHANNEL_QUEUE_CAPACITY 16u

typedef struct channel channel_t;

typedef struct channel_message {
    uint32_t type;
    uint32_t sender_pid;
    uint32_t value;
} channel_message_t;

void channel_init(void);
channel_t* channel_create(void);
bool channel_send(channel_t* channel, const channel_message_t* message);
bool channel_recv(channel_t* channel, channel_message_t* out_message);
uint32_t channel_get_id(const channel_t* channel);
uint32_t channel_get_owner_pid(const channel_t* channel);
uint32_t channel_get_count(const channel_t* channel);
uint32_t channel_get_waiting_receiver_count(const channel_t* channel);
task_struct_t* channel_peek_waiting_receiver(const channel_t* channel);
uint32_t channel_get_waiting_sender_count(const channel_t* channel);
task_struct_t* channel_peek_waiting_sender(const channel_t* channel);
