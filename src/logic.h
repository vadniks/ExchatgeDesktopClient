
#pragma once

#include <stdbool.h>
#include "defs.h"
#include "collections/list.h"

typedef void (*LogicAsyncTask)(void (*action)(void));
typedef void (*LogicDelayedTask)(unsigned seconds, void (*action)(void));

void logicInit(unsigned argc, const char** argv, LogicAsyncTask asyncTask, LogicDelayedTask delayedTask);
bool logicIsAdminMode(void);
void logicNetListen(void); // causes net module to listen for connection updates
const List* logicUsersList(void); // returns permanent users list in which actual user objects will be inserted/updated/removed later by the net module
const List* logicMessagesList(void); // same as usersList but for conversation messages between users
void logicOnCredentialsReceived(const char* username, const char* password, bool logIn);
void logicCredentialsRandomFiller(char* credentials, unsigned size);
void logicOnLoginRegisterPageQueriedByUser(bool logIn);
void logicOnUserForConversationChosen(unsigned id, RenderConversationChooseVariants chooseVariant);
void logicOnServerShutdownRequested(void);
void logicOnReturnFromConversationPageRequested(void);
char* logicMillisToDateTime(unsigned long millis); // result is a null-terminated formatted string deallocation of which must be performed by the caller
unsigned long logicCurrentTimeMillis(void);
void logicOnSendClicked(const char* message);
void logicOnUpdateUsersListClicked(void);
void logicClean(void);
