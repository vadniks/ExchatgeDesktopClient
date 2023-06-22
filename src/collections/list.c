
#include <sdl/SDL.h>
#include <assert.h>
#include "list.h"

STATIC_CONST_UNSIGNED VOID_PTR_SIZE = sizeof(void*);
STATIC_CONST_UNSIGNED MAX_SIZE = 0xfffffffe;

struct List_t {
    void** values;
    unsigned size;
};

List* listInit(void) {
    List* list = SDL_malloc(sizeof *list);
    list->values = NULL;
    list->size = 0;
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

void listIterateOver(const List* list, bool fromStart, void (*action)(void*)) {
    assert(list && action);

    if (fromStart)
        for (unsigned i = 0; i < list->size; (*action)(list->values[i++]));
    else
        for (unsigned i = list->size > 0 ? list->size - 1 : 0; i > 0; (*action)(list->values[i--]));
}

void listDestroy(List* list) {
    assert(list);
    SDL_free(list->values);
    SDL_free(list);
}
