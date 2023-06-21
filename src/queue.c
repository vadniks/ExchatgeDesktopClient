
#include <sdl/SDL.h>
#include <assert.h>
#include "queue.h"

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

void queuePush(Queue* queue, void* value) {
    assert(queue && queue->size < 0xfffffffe);
    queue->values = SDL_realloc(queue->values, ++(queue->size) * sizeof(void*));
    queue->values[queue->size - 1] = value;
}

void* queuePop(Queue* queue) {
    assert(queue && queue->values);
    void* value = queue->values[--(queue->size)];
    queue->values = SDL_realloc(queue->values, queue->size * sizeof(void*));
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
