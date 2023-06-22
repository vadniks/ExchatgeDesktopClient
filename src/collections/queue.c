
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "queue.h"

STATIC_CONST_UNSIGNED VOID_PTR_SIZE = sizeof(void*);

struct Queue_t {
    void** values;
    unsigned size;
    QueueDeallocator nullable deallocator;
};

Queue* queueInit(QueueDeallocator nullable deallocator) {
    Queue* queue = SDL_malloc(sizeof *queue);
    queue->values = NULL;
    queue->size = 0;
    queue->deallocator = deallocator;
    return queue;
}

void queuePush(Queue* queue, void* nullable value) {
    assert(queue && queue->size < 0xfffffffe);
    queue->values = SDL_realloc(queue->values, ++(queue->size) * VOID_PTR_SIZE);
    queue->values[queue->size - 1] = value;
}

void* nullable queuePop(Queue* queue) {
    assert(queue && queue->values && queue->size > 0);
    void* value = queue->values[--(queue->size)];
    queue->values = SDL_realloc(queue->values, queue->size * VOID_PTR_SIZE);
    return value;
}

unsigned queueSize(const Queue* queue) {
    assert(queue);
    return queue->size;
}

static void destroyValuesIfNotEmpty(Queue* queue) {
    if (!queue->deallocator) return;
    for (unsigned i = 0; i < queue->size; queue->deallocator(queue->values[i++]));
}

void queueDestroy(Queue* queue) {
    assert(queue);
    destroyValuesIfNotEmpty(queue);
    SDL_free(queue->values);
    SDL_free(queue);
}
