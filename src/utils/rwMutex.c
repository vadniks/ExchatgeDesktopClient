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

#include <SDL_stdinc.h>
#include <SDL_mutex.h>
#include <assert.h>
#include <stdbool.h>
#include "../defs.h"
#include "rwMutex.h"

struct RWMutex_t {
    SDL_mutex* mutex;
    atomic unsigned counter;
    atomic bool writeLocked;
};

RWMutex* rwMutexInit(void) {
    RWMutex* rwMutex = SDL_malloc(sizeof *rwMutex);
    rwMutex->mutex = SDL_CreateMutex();
    rwMutex->counter = 0;
    rwMutex->writeLocked = false;
    return rwMutex;
}

void rwMutexReadLock(RWMutex* rwMutex) {
    if (++(rwMutex->counter) == 1)
        assert(!SDL_LockMutex(rwMutex->mutex));
}

void rwMutexReadUnlock(RWMutex* rwMutex) {
    assert(rwMutex->counter);

    if (--(rwMutex->counter) == 0)
        assert(!SDL_UnlockMutex(rwMutex->mutex));
}

void rwMutexWriteLock(RWMutex* rwMutex) {
    assert(!rwMutex->counter);

    assert(!SDL_LockMutex(rwMutex->mutex));
    rwMutex->writeLocked = true;
}

void rwMutexWriteUnlock(RWMutex* rwMutex) {
    assert(!rwMutex->counter && rwMutex->writeLocked);

    rwMutex->writeLocked = false;
    assert(!SDL_UnlockMutex(rwMutex->mutex));
}

void rwMutexDestroy(RWMutex* rwMutex) {
    SDL_DestroyMutex(rwMutex->mutex);
    SDL_free(rwMutex);
}
