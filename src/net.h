
#pragma once

#include <stdbool.h>
#include "defs.h"

bool netInit(Function onMessageReceived); // takes on message processing callback which takes on message body which is deallocated on the callback caller side
unsigned netMessageSize();
void netListen();
void netSend(byte* message, unsigned size);
void netClean();
