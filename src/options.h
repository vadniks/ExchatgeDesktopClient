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

typedef long (*OptionsHostIdSupplier)(void);

bool optionsInit(unsigned usernameSize, unsigned passwordSize, OptionsHostIdSupplier hostIdSupplier);
bool optionsIsAdmin(void);
const char* optionsHost(void);
unsigned optionsPort(void);
const byte* optionsServerSignPublicKey(void);
unsigned optionsServerSignPublicKeySize(void);
char* nullable optionsCredentials(void);
void optionsSetCredentials(const char* nullable credentials); // if null - fills the buffer with random bytes
void optionsClear(void);
