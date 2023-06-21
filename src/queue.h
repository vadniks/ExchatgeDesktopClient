
#pragma once

struct Queue_t;
typedef struct Queue_t Queue;

Queue* queueInit(void);
void queuePush(Queue* queue, void* value);
void* queuePop(Queue* queue);
unsigned queueSize(const Queue* queue);
void queueDestroy(Queue* queue);
