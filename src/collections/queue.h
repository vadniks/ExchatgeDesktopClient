
#pragma once

#include "defs.h"

struct Queue_t;
typedef struct Queue_t Queue;

typedef void (*QueueDeallocator)(void*);

Queue* queueInit(QueueDeallocator nullable deallocator);
void queuePush(Queue* queue, void* nullable value);
void* nullable queuePop(Queue* queue);
unsigned queueSize(const Queue* queue);
void queueDestroy(Queue* queue); // all values that are still remain inside a queue at a time destroy is called are deallocated via supplied deallocator if it's not null
