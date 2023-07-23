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

typedef void (*LifecycleAsyncActionFunction)(void* nullable); // parameter is nullable as it's optional, parameter must be deallocated by the action

bool lifecycleInit(unsigned argc, const char** argv);
void lifecycleLoop(void);
void lifecycleAsync(LifecycleAsyncActionFunction function, void* nullable parameter, unsigned long delayMillis); // delay can be zero in which case no delay is happened
void lifecycleClean(void);
