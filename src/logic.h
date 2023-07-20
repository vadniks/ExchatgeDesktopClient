
#pragma once

#include <stdbool.h>
#include "defs.h"
#include "collections/list.h"

void logicInit(unsigned argc, const char** argv);
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
void logicOnSendClicked(const char* text, unsigned size); // expects a string with 'size' in range (0, NET_MESSAGE_BODY_SIZE] which is copied
void logicOnUpdateUsersListClicked(void);
unsigned logicUnencryptedMessageBodySize(void);
void logicClean(void);
