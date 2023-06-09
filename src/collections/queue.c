
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

void queuePush(Queue* queue, void* value) {
    assert(queue && queue->size < 0xfffffffe);
    queue->values = SDL_realloc(queue->values, ++(queue->size) * VOID_PTR_SIZE);
    queue->values[queue->size - 1] = value;
}

void* queuePop(Queue* queue) {
    assert(queue && queue->values && queue->size > 0);
    void* value = queue->values[0];

    const unsigned newSize = queue->size - 1;
    if (!newSize) {
        SDL_free(queue->values);
        queue->values = NULL;
        queue->size = 0;
        return value;
    }

    void** temp = SDL_malloc(newSize * VOID_PTR_SIZE);
    SDL_memcpy(temp, &(queue->values[1]), newSize * VOID_PTR_SIZE);

    SDL_free(queue->values);
    queue->values = temp;

    queue->size = newSize;
    return value;
}

unsigned queueSize(const Queue* queue) {
    assert(queue);
    return queue->size;
}

static void destroyValuesIfNotEmpty(Queue* queue) {
    if (!queue->deallocator) return;
    for (unsigned i = 0; i < queue->size; (*(queue->deallocator))(queue->values[i++]));
}

void queueDestroy(Queue* queue) {
    assert(queue);
    destroyValuesIfNotEmpty(queue);
    SDL_free(queue->values);
    SDL_free(queue);
}
