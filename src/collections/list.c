
#include <sdl/SDL_stdinc.h>
#include <sdl/SDL_mutex.h>
#include <assert.h>
#include "list.h"

#define SYNCHRONIZED_BEGIN assert(!SDL_LockMutex(list->mutex));
#define SYNCHRONIZED_END assert(!SDL_UnlockMutex(list->mutex));

STATIC_CONST_UNSIGNED VOID_PTR_SIZE = sizeof(void*);
STATIC_CONST_UNSIGNED MAX_SIZE = 0xfffffffe;

struct List_t {
    void** values;
    atomic unsigned size;
    ListDeallocator nullable deallocator;
    SDL_mutex* mutex;
    atomic bool destroyed;
};

List* listInit(ListDeallocator nullable deallocator) {
    List* list = SDL_malloc(sizeof *list);
    list->values = NULL;
    list->size = 0;
    list->deallocator = deallocator;
    list->mutex = SDL_CreateMutex();
    list->destroyed = false;
    return list;
}

void listAddBack(List* list, void* value) {
    assert(list && !list->destroyed);
    SYNCHRONIZED_BEGIN
    assert(list->size < MAX_SIZE);

    list->values = SDL_realloc(list->values, ++(list->size) * VOID_PTR_SIZE);
    list->values[list->size - 1] = value;

    SYNCHRONIZED_END
}

void listAddFront(List* list, void* value) {
    assert(list && !list->destroyed);
    SYNCHRONIZED_BEGIN
    assert(list->size < MAX_SIZE);

    void** temp = SDL_malloc(++(list->size) * VOID_PTR_SIZE);
    temp[0] = value;
    for (unsigned i = 1; i < list->size; temp[i] = (list->values)[i - 1], i++);

    SDL_free(list->values);
    list->values = temp;

    SYNCHRONIZED_END
}

const void* listGet(const List* list, unsigned index) {
    assert(list && !list->destroyed);
    SYNCHRONIZED_BEGIN
    assert(list->values && list->size > 0 && list->size < MAX_SIZE && index < MAX_SIZE);

    const void* value = list->values[index];
    SYNCHRONIZED_END
    return value;
}

unsigned listSize(const List* list) {
    assert(list && !list->destroyed);
    return list->size;
}

const void* nullable listBinarySearch(const List* list, const void* key, ListComparator comparator) {
    assert(list && !list->destroyed);
    SYNCHRONIZED_BEGIN
    assert(list->values && list->size > 0);

    const unsigned long index = (void**) SDL_bsearch(key, list->values, list->size, VOID_PTR_SIZE, comparator) - list->values;
    const void* value = index >= list->size ? NULL : list->values[index];

    SYNCHRONIZED_END
    return value;
}

static void destroyValuesIfNotEmpty(List* list) {
    if (!list->deallocator) return;
    assert(list->size && list->values || !(list->size) && !(list->values));
    for (unsigned i = 0; i < list->size; (*(list->deallocator))(list->values[i++]));
}

void listClear(List* list) {
    assert(list && !list->destroyed);
    SYNCHRONIZED_BEGIN

    destroyValuesIfNotEmpty(list);
    list->size = 0;

    SDL_free(list->values);
    list->values = NULL;

    SYNCHRONIZED_END
}

void listDestroy(List* list) {
    assert(list && !list->destroyed);
    list->destroyed = true;

    destroyValuesIfNotEmpty(list);
    SDL_free(list->values);
    SDL_DestroyMutex(list->mutex);
    SDL_free(list);
}
