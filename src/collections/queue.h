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

#include "defs.h"

struct Queue_t;
typedef struct Queue_t Queue; // thread-safe

typedef void (*QueueDeallocator)(void*);
typedef unsigned long (*QueueCurrentTimeMillisGetter)(void);

Queue* queueInitExtra(QueueDeallocator nullable deallocator, QueueCurrentTimeMillisGetter nullable currentTimeMillisGetter);

inline Queue* queueInit(QueueDeallocator nullable deallocator)
{ return queueInitExtra(deallocator, NULL); }

void queuePush(Queue* queue, const void* value);
void* queuePop(Queue* queue); // returns stored value that must be deallocated by a caller as reference to the value gets deleted, the queue must not be empty
void* nullable queueWaitAndPop(Queue* queue, int timeout); // works (blocks the caller thread) as the plain pop if the queue is not empty, waits until smth is pushed into it and then returns the newly pushed value if the queue is empty, timeout can be negative in which case the function will wait indefinitely, when timeout exceeds without receiving any value then null will be returned
void* nullable queuePeek(Queue* queue); // returns the top value if the queue is not empty and null if it is but doesn't remove the top value
void queueDropTop(Queue* queue); // drops the top value and deallocates it
unsigned queueSize(const Queue* queue); // Queue is const here 'cause it's mutex isn't modified
void queueClear(Queue* queue);
void queueDestroy(Queue* queue); // all values that are still remain inside a queue at a time destroy is called are deallocated via supplied deallocator if it's not null
