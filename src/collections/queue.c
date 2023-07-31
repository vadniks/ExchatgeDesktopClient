/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <SDL_stdinc.h>
#include <SDL_mutex.h>
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

void queuePush(Queue* queue, const void* value) {
    assert(queue && !queue->destroyed);
    SYNCHRONIZED_BEGIN
    assert(queue->size < 0xfffffffe);

    queue->values = SDL_realloc(queue->values, ++(queue->size) * VOID_PTR_SIZE);
    queue->values[queue->size - 1] = (void*) value;

    SYNCHRONIZED_END
}

void* queuePop(Queue* queue) {
    assert(queue && !queue->destroyed && queue->size > 0);
    SYNCHRONIZED_BEGIN
    assert(queue->values);

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
