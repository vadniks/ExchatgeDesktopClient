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

#define RW_MUTEX_READ_LOCKED(m, a) rwMutexReadLock(m); a rwMutexReadUnlock(m);
#define RW_MUTEX_WRITE_LOCKED(m, a) rwMutexWriteLock(m); a rwMutexWriteUnlock(m);

struct RWMutex_t;
typedef struct RWMutex_t RWMutex;

RWMutex* rwMutexInit(void);
void rwMutexReadLock(RWMutex* rwMutex);
void rwMutexReadUnlock(RWMutex* rwMutex);
void rwMutexWriteLock(RWMutex* rwMutex);
void rwMutexWriteUnlock(RWMutex* rwMutex);
void rwMutexDestroy(RWMutex* rwMutex);
