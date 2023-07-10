
#pragma once

#include <stdbool.h>
#include "defs.h"

typedef void (*LifecycleAsyncActionFunction)(void* nullable); // parameter is nullable as it's optional, parameter must be deallocated by the action

bool lifecycleInit(unsigned argc, const char** argv);
void lifecycleLoop(void);
void lifecycleAsync(LifecycleAsyncActionFunction function, void* nullable parameter, unsigned long delayMillis); // delay can be zero in which case no delay is happened
void lifecycleClean(void);
