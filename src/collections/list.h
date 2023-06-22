
#pragma once

#include <stdbool.h>
#include "defs.h"

struct List_t;
typedef struct List_t List;

List* listInit(void);
void listAdd(List* list, void* nullable value);
void* nullable listGet(const List* list, unsigned index);
unsigned listSize(const List* list);
void listIterateOver(const List* list, bool fromStart, void (*action)(void*));
void listDestroy(List* list); // all values inside a list must be deallocated before calling this function
