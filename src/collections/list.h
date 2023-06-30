
#pragma once

#include <stdbool.h>
#include "defs.h"

struct List_t;
typedef struct List_t List;

typedef void (*ListDeallocator)(void*);

List* listInit(ListDeallocator nullable deallocator);
void listAdd(List* list, void* nullable value);
void* nullable listGet(const List* list, unsigned index);
unsigned listSize(const List* list);
void listIterateOver(const List* list, bool fromStart, void (*action)(void*));
void listClear(List* list);
void listDestroy(List* list); // all values that are still remain inside a queue at a time destroy is called are deallocated via supplied deallocator if it's not null
