
#pragma once

#include "defs.h"
#include <stdbool.h>

void logicInit(void);
void logicNetListen(void); // causes net module to listen for connection updates
void logicOnCredentialsReceived(const char* username, const char* password, bool logIn);
void logicCredentialsRandomFiller(char* credentials, unsigned size);
void logicOnLoginRegisterPageQueriedByUser(bool logIn);
void logicClean(void);
