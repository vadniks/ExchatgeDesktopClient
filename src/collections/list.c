
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "list.h"

STATIC_CONST_UNSIGNED VOID_PTR_SIZE = sizeof(void*);
STATIC_CONST_UNSIGNED MAX_SIZE = 0xfffffffe;

struct List_t {
    void** values;
    unsigned size;
    ListDeallocator nullable deallocator;
};

List* listInit(ListDeallocator nullable deallocator) {
    List* list = SDL_malloc(sizeof *list);
    list->values = NULL;
    list->size = 0;
    list->deallocator = deallocator;
    return list;
}

void listAdd(List* list, void* nullable value) {
    assert(list && list->size < MAX_SIZE);
    list->values = SDL_realloc(list->values, ++(list->size) * VOID_PTR_SIZE);
    list->values[list->size - 1] = value;
}

void* nullable listGet(const List* list, unsigned index) {
    assert(list && list->values && list->size > 0 && list->size < MAX_SIZE && index < MAX_SIZE);
    return list->values[index];
}

unsigned listSize(const List* list) {
    assert(list);
    return list->size;
}

static void destroyValuesIfNotEmpty(List* list) {
    if (!list->deallocator) return;
    for (unsigned i = 0; i < list->size; (*(list->deallocator))(list->values[i++]));
}

void listClear(List* list) {
    assert(list);

    destroyValuesIfNotEmpty(list);
    list->size = 0;

    SDL_free(list->values);
    list->values = NULL;
}

void listDestroy(List* list) {
    assert(list);
    destroyValuesIfNotEmpty(list);
    SDL_free(list->values);
    SDL_free(list);
}
