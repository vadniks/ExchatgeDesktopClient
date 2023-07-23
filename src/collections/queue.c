
#include <sdl/SDL_stdinc.h>
#include <sdl/SDL_mutex.h>
#include <assert.h>
#include <stdbool.h>
#include "queue.h"

#define SYNCHRONIZED_BEGIN assert(!SDL_LockMutex(queue->mutex));
#define SYNCHRONIZED_END assert(!SDL_UnlockMutex(queue->mutex));

STATIC_CONST_UNSIGNED VOID_PTR_SIZE = sizeof(void*);

struct Queue_t {
    void** values;
    atomic unsigned size;
    QueueDeallocator nullable deallocator;
    SDL_mutex* mutex;
    atomic bool destroyed;
};

Queue* queueInit(QueueDeallocator nullable deallocator) {
    Queue* queue = SDL_malloc(sizeof *queue);
    queue->values = NULL;
    queue->size = 0;
    queue->deallocator = deallocator;
    queue->mutex = SDL_CreateMutex();
    queue->destroyed = false;
    return queue;
}

void queuePush(Queue* queue, void* value) {
    assert(queue && !queue->destroyed);
    SYNCHRONIZED_BEGIN
    assert(queue->size < 0xfffffffe);

    queue->values = SDL_realloc(queue->values, ++(queue->size) * VOID_PTR_SIZE);
    queue->values[queue->size - 1] = value;

    SYNCHRONIZED_END
}

void* queuePop(Queue* queue) {
    assert(queue && !queue->destroyed);
    SYNCHRONIZED_BEGIN
    assert(queue->values && queue->size > 0);

    void* value = queue->values[0];

    const unsigned newSize = queue->size - 1;
    if (!newSize) {
        SDL_free(queue->values);
        queue->values = NULL;
        queue->size = 0;
        SYNCHRONIZED_END
        return value;
    }

    void** temp = SDL_malloc(newSize * VOID_PTR_SIZE);
    SDL_memcpy(temp, &(queue->values[1]), newSize * VOID_PTR_SIZE);

    SDL_free(queue->values);
    queue->values = temp;

    queue->size = newSize;

    SYNCHRONIZED_END
    return value;
}

unsigned queueSize(const Queue* queue) {
    assert(queue && !queue->destroyed);
    return queue->size;
}

static void destroyValuesIfNotEmpty(Queue* queue) {
    if (!queue->deallocator) return;
    assert(queue->size && queue->values || !(queue->size) && !(queue->values));
    for (unsigned i = 0; i < queue->size; (*(queue->deallocator))(queue->values[i++]));
}

void queueDestroy(Queue* queue) {
    assert(queue && !queue->destroyed);
    queue->destroyed = true;

    destroyValuesIfNotEmpty(queue);
    SDL_free(queue->values);
    SDL_DestroyMutex(queue->mutex);
    SDL_free(queue);
}
