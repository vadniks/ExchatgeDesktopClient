
#pragma once

#include <SDL.h>
#include <stdio.h>
#include "../src/defs.h"
#include "../src/collections/list.h"

static int testCollections_list_comparator(const unsigned* left, const unsigned** right)
{ return *left < **right ? -1 : (*left > **right ? 1 : 0); }

void testCollections_list(void) {
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

    const unsigned* searched = listBinarySearch(list, (unsigned[]) {0}, (ListComparator) &testCollections_list_comparator);
    assert(searched && !*searched);

    listDestroy(list);

    assert(allocations == SDL_GetNumAllocations());
}
