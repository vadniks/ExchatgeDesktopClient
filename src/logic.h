
#pragma once

#include <stdbool.h>
#include "defs.h"
#include "collections/list.h"

typedef void (*LogicAsyncTaskLauncher)(void (*action)(void));

void logicInit(LogicAsyncTaskLauncher asyncTaskLauncher);
void logicNetListen(void); // causes net module to listen for connection updates
const List* logicUsersList(void); // returns permanent users list in which actual user objects will be inserted/updated/removed later by the net module
void logicOnCredentialsReceived(const char* username, const char* password, bool logIn);
void logicCredentialsRandomFiller(char* credentials, unsigned size);
void logicOnLoginRegisterPageQueriedByUser(bool logIn);
void logicOnUserForConversationChosen(RenderConversationChooseVariants chooseVariant);
void logicClean(void);
