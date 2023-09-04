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

#include "defs.h"

struct Queue_t;
typedef struct Queue_t Queue; // thread-safe

typedef void (*QueueDeallocator)(void*);

Queue* queueInit(QueueDeallocator nullable deallocator);
void queuePush(Queue* queue, const void* value);
void* queuePop(Queue* queue); // returns stored value that must be deallocated by a caller as reference to the value gets deleted
unsigned queueSize(const Queue* queue); // Queue is const here 'cause it's mutex isn't modified
void queueDestroy(Queue* queue); // all values that are still remain inside a queue at a time destroy is called are deallocated via supplied deallocator if it's not null
