
#pragma once

#include <stdbool.h>

bool lifecycleInit(unsigned argc, const char** argv);
void lifecycleLoop(void);
void lifecycleClean(void);
