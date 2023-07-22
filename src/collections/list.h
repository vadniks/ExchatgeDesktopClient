
#pragma once

#include <stdbool.h>
#include "defs.h"

struct List_t;
typedef struct List_t List;

typedef void (*ListDeallocator)(void*);
typedef int (*ListComparator)(const void*, const void*);

List* listInit(ListDeallocator nullable deallocator);
void listAddBack(List* list, void* value);
void listAddFront(List* list, void* value);
void* listGet(const List* list, unsigned index);
unsigned listSize(const List* list);
const void* nullable listBinarySearch(const List* list, const void* key, ListComparator comparator);
void listClear(List* list);
void listDestroy(List* list); // all values that are still remain inside a queue at a time destroy is called are deallocated via supplied deallocator if it's not null
