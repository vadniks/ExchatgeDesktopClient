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

#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <SDL.h>
#include "../src/collections/list.h"
#include "../src/collections/queue.h"

static int testCollections_listBasicComparator(const unsigned* left, const unsigned** right)
{ return *left < **right ? -1 : (*left > **right ? 1 : 0); }

void testCollections_listBasic(void) {
    const int allocations = SDL_GetNumAllocations();

    List* list = listInit(&SDL_free);
    const unsigned count = 5;

    for (unsigned i = 0; i < count; i++) {
        unsigned* new = SDL_malloc(sizeof(int));
        *new = i;
        listAddBack(list, new);
    }

    for (unsigned i = 0; i < count; i++)
        assert(*((unsigned*) listGet(list, i)) == i);

    assert(listSize(list) == count);

    const unsigned* searched = listBinarySearch(list, (unsigned[]) {0}, (ListComparator) &testCollections_listBasicComparator);
    assert(searched && !*searched);

    listDestroy(list);

    assert(allocations == SDL_GetNumAllocations());
}

static void* testCollections_listExtraItemDuplicator(const void* value) { return (void*) value; }

void testCollections_listExtra(void) {
    const int allocations = SDL_GetNumAllocations();

    List* list = listInit(NULL);
    const unsigned count = 5;

    for (unsigned i = 0; i < count; i++)
        listAddFront(list, (void*) (long) i);

    List* list2 = listCopy(list, &testCollections_listExtraItemDuplicator);

    listClear(list);
    assert(!listSize(list));
    listDestroy(list);

    assert(listSize(list2) == count);
    for (unsigned i = 0, j = count - 1; i < count; i++, j--)
        assert(listGet(list2, i) == (void*) (long) j);

    listDestroy(list2);

    assert(allocations == SDL_GetNumAllocations());
}

void testCollections_queueBasic(void) {
    const int allocations = SDL_GetNumAllocations();

    Queue* queue = queueInit(&SDL_free);
    const unsigned count = 5;

    for (unsigned i = 0; i < count; i++) {
        unsigned* j = SDL_malloc(sizeof *j);
        *j = i;
        queuePush(queue, j);
    }

    for (unsigned i = 0; queuePeek(queue); i++) {
        unsigned* j = queuePop(queue);
        assert(j && *j == i);
        SDL_free(j);
    }

    assert(!queueSize(queue));
    queueDestroy(queue);

    assert(allocations == SDL_GetNumAllocations());
}

int testCollections_queueExtraListener(void* queue) {
    sleep(1);
    unsigned* value = SDL_malloc(sizeof *value);
    *value = 0;
    queuePush(queue, value);
    return 0;
}

static unsigned long currentTimeMillis(void) {
    struct timespec timespec;
    assert(!clock_gettime(CLOCK_REALTIME, &timespec));
    return timespec.tv_sec * (unsigned) 1e3f + timespec.tv_nsec / (unsigned) 1e6f;
}

void testCollections_queueExtra(void) {
    const int allocations = SDL_GetNumAllocations();

    Queue* queue = queueInitExtra(&SDL_free, &currentTimeMillis);
    SDL_Thread* listener = SDL_CreateThread(&testCollections_queueExtraListener, "0", queue);

    unsigned* value = queueWaitAndPop(queue, 2000);
    assert(value && !*value);
    SDL_free(value);

    SDL_WaitThread(listener, NULL);
    queueDestroy(queue);

    assert(allocations == SDL_GetNumAllocations());
}
