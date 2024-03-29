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

#include <stdbool.h>
#include "defs.h"

typedef long (*OptionsHostIdSupplier)(void);

typedef enum : byte {
    OPTIONS_THEME_LIGHT = 0,
    OPTIONS_THEME_DARK = 1
} OptionsThemes;

typedef enum : byte {
    OPTIONS_LANGUAGE_ENGLISH = 0,
    OPTIONS_LANGUAGE_RUSSIAN = 1
} OptionsLanguages;

bool optionsInit(unsigned usernameSize, unsigned passwordSize, OptionsHostIdSupplier hostIdSupplier);
bool optionsIsAdmin(void);
const char* optionsHost(void);
unsigned optionsPort(void);
const byte* optionsServerSignPublicKey(void);
unsigned optionsServerSignPublicKeySize(void);
OptionsThemes optionsTheme(void);
OptionsLanguages optionsLanguage(void);
const char* nullable optionsCredentials(void); // TODO: move to database
void optionsSetCredentials(const char* nullable credentials); // if null - removes the option's payload from file
void optionsClean(void); // buffer in which the credentials are stored gets overwritten with random data at module's cleanup
