
#pragma once

#include <stdbool.h>
#include "../defs.h"

bool databaseInit(byte* passwordBuffer, unsigned size); // returns true on success, 'size'-sized buffer is filled with random bytes after initiating and is freed afterwords
bool databaseAddUser(unsigned id);
bool databaseGetUser(unsigned id);
void databaseClean(void);
