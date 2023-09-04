/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)
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

#include <stdbool.h>
#include "defs.h"

struct List_t;
typedef struct List_t List; // thread-safe

typedef void (*ListDeallocator)(void*);
typedef int (*ListComparator)(const void*, const void*);

List* listInit(ListDeallocator nullable deallocator);
void listAddBack(List* list, const void* value);
void listAddFront(List* list, const void* value);
const void* listGet(List* list, unsigned index); // list is actually treated as non-const 'cause mutex inside it is being modified
unsigned listSize(const List* list); // but not here as mutex isn't modified here
const void* nullable listBinarySearch(List* list, const void* key, ListComparator comparator);
void listClear(List* list);
void listDestroy(List* list); // all values that are still remain inside a queue at a time destroy is called are deallocated via supplied deallocator if it's not null
