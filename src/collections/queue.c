
#include <sdl/SDL.h>
#include <assert.h>
#include "queue.h"

STATIC_CONST_UNSIGNED VOID_PTR_SIZE = sizeof(void*);

struct Queue_t {
    void** values;
    unsigned size;
};

Queue* queueInit(void) {
    Queue* queue = SDL_malloc(sizeof *queue);
    queue->values = NULL;
    queue->size = 0;
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

void queueDestroy(Queue* queue) {
    assert(queue);
    SDL_free(queue->values);
    SDL_free(queue);
}
