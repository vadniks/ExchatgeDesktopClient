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

#pragma once

#include <SDL.h>
#include <stdio.h>
#include "../src/defs.h"
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
