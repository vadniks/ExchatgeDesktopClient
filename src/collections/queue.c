/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023-2024  Vadim Nikolaev (https://github.com/vadniks)
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
#include <assert.h>
#include <stdbool.h>
#include "../utils/rwMutex.h"
#include "queue.h"

STATIC_CONST_UNSIGNED VOID_PTR_SIZE = sizeof(void*);

struct Queue_t {
    void** values;
    atomic unsigned size;
    QueueDeallocator nullable deallocator;
    RWMutex* rwMutex;
    atomic bool destroyed;
    QueueCurrentTimeMillisGetter nullable currentTimeMillisGetter;
};

Queue* queueInitExtra(QueueDeallocator nullable deallocator, QueueCurrentTimeMillisGetter nullable currentTimeMillisGetter) {
    Queue* queue = SDL_malloc(sizeof *queue);
    queue->values = NULL;
    queue->size = 0;
    queue->deallocator = deallocator;
    queue->rwMutex = rwMutexInit();
    queue->destroyed = false;
    queue->currentTimeMillisGetter = currentTimeMillisGetter;
    return queue;
}

void queuePush(Queue* queue, const void* value) {
    assert(queue && !queue->destroyed && queue->size < 0xfffffffe);

    RW_MUTEX_WRITE_LOCKED(queue->rwMutex,
        queue->values = SDL_realloc(queue->values, ++(queue->size) * VOID_PTR_SIZE);
        queue->values[queue->size - 1] = (void*) value;
    )
}

void* queuePop(Queue* queue) {
    assert(queue && !queue->destroyed && queue->size > 0);
    rwMutexWriteLock(queue->rwMutex);
    assert(queue->values);

    void* value = queue->values[0];

    const unsigned newSize = queue->size - 1;
    if (!newSize) {
        SDL_free(queue->values);
        queue->values = NULL;
        queue->size = 0;
        rwMutexWriteUnlock(queue->rwMutex);
        return value;
    }

    void** temp = SDL_malloc(newSize * VOID_PTR_SIZE);
    SDL_memcpy(temp, &(queue->values[1]), newSize * VOID_PTR_SIZE);

    SDL_free(queue->values);
    queue->values = temp;
    queue->size = newSize;

    rwMutexWriteUnlock(queue->rwMutex);
    return value;
}

void* nullable queueWaitAndPop(Queue* queue, int timeout) {
    assert(queue && !queue->destroyed);
    if (timeout >= 0) assert(queue->currentTimeMillisGetter);

    const unsigned long startMillis = (*(queue->currentTimeMillisGetter))();

    while (timeout >= 0 ? !queue->destroyed && (*(queue->currentTimeMillisGetter))() - startMillis < (unsigned long) timeout : true)
        if (queueSize(queue)) // TODO: add mutex lock here
            return queuePop(queue); // TODO: assert not null

    return NULL;
}

void* nullable queuePeek(Queue* queue) {
    assert(queue && !queue->destroyed);
    void* result = NULL;

    RW_MUTEX_READ_LOCKED(queue->rwMutex,
        if (queue->size)
            result = queue->values[0];
    )

    return result;
}

void queueDropTop(Queue* queue) { (*(queue->deallocator))(queuePop(queue)); }

unsigned queueSize(const Queue* queue) {
    assert(queue && !queue->destroyed);
    return queue->size;
}

static void destroyValuesIfNotEmpty(Queue* queue) {
    if (!queue->deallocator) return;
    assert(queue->size && queue->values || !(queue->size) && !(queue->values));
    for (unsigned i = 0; i < queue->size; (*(queue->deallocator))(queue->values[i++]));
}

void queueClear(Queue* queue) {
    assert(queue && !queue->destroyed);

    RW_MUTEX_WRITE_LOCKED(queue->rwMutex,
        destroyValuesIfNotEmpty(queue);
        queue->size = 0;

        SDL_free(queue->values);
        queue->values = NULL;
    )
}

void queueDestroy(Queue* queue) {
    assert(queue && !queue->destroyed);
    queue->destroyed = true;

    destroyValuesIfNotEmpty(queue);
    SDL_free(queue->values);
    rwMutexDestroy(queue->rwMutex);
    SDL_free(queue);
}
