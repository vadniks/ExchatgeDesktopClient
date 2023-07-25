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

#include <sdl/SDL.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include "render/render.h"
#include "crypto.h"
#include "net.h"
#include "user.h"
#include "conversationMessage.h"
#include "lifecycle.h"
#include "database/database.h"
#include "logic.h"

typedef enum : unsigned {
    STATE_UNAUTHENTICATED = 0,
    STATE_AWAITING_AUTHENTICATION = 1,
    STATE_AUTHENTICATED = 2,
    STATE_EXCHANGING_MESSAGES = 3
} States;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    atomic bool netInitialized;
    List* usersList; // <User*>
    List* messagesList; // <ConversationMessage*>
    char* executablePath;
    unsigned executablePathSize;
    bool adminMode;
    atomic unsigned state;
    char* currentUserName;
    unsigned toUserId; // the id of the user, the current user (logged in via this client) wanna speak to
    atomic bool databaseInitialized;
)
#pragma clang diagnostic pop

static void parseArguments(unsigned argc, const char** argv) {
    assert(argc <= 1 || argc == 2 && argv[1]); // 'cause argv[0] is path to the executable everytime

    this->executablePathSize = SDL_strlen(argv[0]);
    assert(this->executablePathSize > 0 && this->executablePathSize <= 0xff);
    this->executablePath = SDL_malloc(this->executablePathSize);
    SDL_memcpy(this->executablePath, argv[0], this->executablePathSize);

    if (argc <= 1) {
        this->adminMode = false;
        return;
    }

    const unsigned patternSize = 7;
    const char pattern[patternSize] = "--admin"; // Just unlock access to admin operations, they're executed on the server after it verifies the caller, so verification on the client side is unnecessary
    this->adminMode = !SDL_memcmp(argv[1], pattern, patternSize);
}

void logicInit(unsigned argc, const char** argv) {
    assert(!this);
    this = SDL_malloc(sizeof *this);
    this->netInitialized = false;
    this->usersList = userInitList();
    this->messagesList = conversationMessageInitList();
    parseArguments(argc, argv);
    this->state = STATE_UNAUTHENTICATED;
    this->currentUserName = SDL_calloc(NET_USERNAME_SIZE, sizeof(char));
    this->databaseInitialized = false;

    lifecycleAsync((LifecycleAsyncActionFunction) &renderShowLogIn, NULL, 1000);
}

bool logicIsAdminMode(void) {
    assert(this);
    return this->adminMode;
}

void logicNetListen(void) {
    if (!this) return; // logic module might haven't been initialized by the time lifecycle module's netThread has started periodically calling this function to update connection
    if (!this->netInitialized) return;
    netListen();
}

const List* logicUsersList(void) {
    assert(this);
    return this->usersList;
}

const List* logicMessagesList(void) {
    assert(this);
    return this->messagesList;
}

static int findUserComparator(const unsigned* xId, const User* const* user)
{ return *xId < (*user)->id ? -1 : (*xId > (*user)->id ? 1 : 0); }

static const User* nullable findUser(unsigned id)
{ return listBinarySearch(this->usersList, &id, (ListComparator) &findUserComparator); }

static void onMessageReceived(unsigned long timestamp, unsigned fromId, const byte* encryptedMessage, unsigned encryptedSize) {
    assert(this && this->databaseInitialized);

    const User* user = findUser(fromId);
    if (!user) return;

    const unsigned size = encryptedSize - cryptoEncryptedSize(0);
    assert(size > 0 && size <= logicUnencryptedMessageBodySize());

    Crypto* crypto = databaseGetConversation(fromId);
    if (!crypto) return;

    byte* message = cryptoDecrypt(crypto, encryptedMessage, encryptedSize, false);
    assert(message);
    cryptoDestroy(crypto);

    DatabaseMessage* dbMessage = databaseMessageCreate(timestamp, fromId, fromId, (byte*) message, size);
    databaseAddMessage(dbMessage);
    databaseMessageDestroy(dbMessage);

    if (fromId == this->toUserId)
        listAddFront(this->messagesList, conversationMessageCreate(timestamp, user->name, NET_USERNAME_SIZE, (const char*) message, size));
    SDL_free(message);
}

static void onLogInResult(bool successful) {
    assert(this);
    if (successful) {
        this->state = STATE_AUTHENTICATED;
        netFetchUsers();
    } else {
        this->state = STATE_UNAUTHENTICATED;
        renderShowLogIn();
        renderShowSystemError();
        renderHideInfiniteProgressBar();
    }
}

static void onErrorReceived(__attribute_maybe_unused__ int flag) {
    assert(this);
    renderHideInfiniteProgressBar();
    renderShowSystemError();
}

static void onRegisterResult(bool successful) {
    assert(this);
    successful ? renderShowRegistrationSucceededSystemMessage() : renderShowSystemError();
    renderHideInfiniteProgressBar();
}

static void onDisconnected(void) {
    assert(this);

    this->netInitialized = false;
    this->state = STATE_UNAUTHENTICATED;

    renderHideInfiniteProgressBar();
    renderShowLogIn();
    renderShowDisconnectedError();
}

static void onUsersFetched(NetUserInfo** infos, unsigned size) {
    assert(this && this->databaseInitialized);

    NetUserInfo* info;
    for (unsigned i = 0, id; i < size; i++) {
        info = infos[i];
        id = netUserInfoId(info);

        if (id != netCurrentUserId()) {
            listAddBack(this->usersList, userCreate(
                id,
                (const char*) netUserInfoName(info),
                NET_USERNAME_SIZE,
                databaseConversationExists(id),
                netUserInfoConnected(info)
            ));
        } else {
            SDL_memcpy(this->currentUserName, netUserInfoName(info), NET_USERNAME_SIZE);
            renderSetWindowTitle(this->currentUserName);
        }
    }

    renderHideInfiniteProgressBar();
    renderSetControlsBlocking(false);
    renderShowUsersList(this->currentUserName);
}

static void tryLoadPreviousMessages(unsigned id) {
    assert(this && this->databaseInitialized);
    listClear(this->messagesList);

    List* messages = databaseGetMessages(id);
    if (!messages) return;

    const User* user = findUser(id);
    assert(user);

    const DatabaseMessage* dbMessage;
    bool fromCurrentUser;
    for (unsigned i = 0; i < listSize(messages); i++) {
        dbMessage = listGet(messages, i);
        fromCurrentUser = databaseMessageFrom(dbMessage) == netCurrentUserId();

        listAddBack(this->messagesList, conversationMessageCreate(
            databaseMessageTimestamp(dbMessage),
            fromCurrentUser ? NULL : user->name,
            fromCurrentUser ? 0 : NET_USERNAME_SIZE,
            (const char*) databaseMessageText(dbMessage),
            databaseMessageSize(dbMessage)
        ));
    }

    listDestroy(messages);
}

static void replyToConversationSetUpInvite(unsigned* fromId) {
    assert(this && fromId);
    unsigned xFromId = *fromId;
    SDL_free(fromId);

    listClear(this->messagesList);

    if (databaseConversationExists(xFromId))
        databaseRemoveConversation(xFromId); // existence of a conversation when receiving an invite means that user, from whom this invite came, has deleted conversation with the current user, so to make users messaging again, remove the outdated conversation on the current user's side

    const User* user = findUser(xFromId);
    assert(user);

    Crypto* crypto = netReplyToPendingConversationSetUpInvite(renderShowInviteOrRequestDialog(0, user->name), xFromId);
    if (crypto)
        this->toUserId = xFromId, // not only in python there's indentation based scoping, here's an emulation though
        this->state = STATE_EXCHANGING_MESSAGES,
        databaseAddConversation(xFromId, crypto),
        cryptoDestroy(crypto),
        tryLoadPreviousMessages(xFromId),
        renderShowConversation(user->name);
    else
        renderShowUnableToCreateConversation();

    renderSetControlsBlocking(false);
    renderHideInfiniteProgressBar();
}

static void onConversationSetUpInviteReceived(unsigned fromId) {
    unsigned* xFromId = SDL_malloc(sizeof(int));
    *xFromId = fromId;
    renderShowInfiniteProgressBar();
    renderSetControlsBlocking(true);
    lifecycleAsync((LifecycleAsyncActionFunction) &replyToConversationSetUpInvite, xFromId, 0);
}

static void onFileExchangeInviteReceived(unsigned fromId, unsigned fileSize) {
    netReplyToFileExchangeInvite(fromId, fileSize, false); // TODO
}

static unsigned nextFileChunkSupplier(unsigned index, byte* buffer) {
    return 0; // TODO
}

static void nextFileChunkReceiver(unsigned index, unsigned fileSize, unsigned receivedBytesCount, const byte* buffer) {
    // TODO
}

void logicFileChooseResultHandler(const char* nullable filePath) {
    // TODO
}

void logicOnFileChooserRequested(void) { // TODO: add checksum
    assert(this);
    renderShowFileChooser();
}

static void filePathDeallocator(char* path) { SDL_free(path); }

__attribute_deprecated_msg__("isn't needed") static bool isDirectory(const char* path) {
    struct stat xStat;
    return stat(path, &xStat) != 0 ? false : S_ISDIR(xStat.st_mode);
}

List* logicFilesListGetter(void) { // returns List*<char*>
    assert(this);

    DIR* dir;
    struct dirent* dirent;
    List* filesPaths = listInit((ListDeallocator) &filePathDeallocator);

    if (!(dir = opendir("."))) return filesPaths;

    char* buffer = NULL;
    const unsigned bufferSize = 0xff;

    while ((dirent = readdir(dir)) != NULL) {
        if (dirent->d_type == DT_REG) {
            buffer = SDL_malloc(bufferSize);
            SDL_strlcpy(buffer, dirent->d_name, bufferSize);
            listAddBack(filesPaths, buffer);
        }
    }
    closedir(dir);

    return filesPaths;
}

char* logicExecutableDirAbsolutePath(void) { // returns executablePath.substring(0, __last_slash_index__ - 1)
    assert(this);
    char* path = SDL_malloc(this->executablePathSize);

    unsigned i = 0, foundAt = 0;
    for (char chr = this->executablePath[i]; chr; chr = this->executablePath[++i]) {
        path[i] = chr;
        if (chr == '/') foundAt = i;
    }

    assert(path && foundAt > 0 && foundAt < this->executablePathSize);

    char* trimmedPath = SDL_malloc(foundAt + 1);
    SDL_memcpy(trimmedPath, path, foundAt);
    trimmedPath[foundAt] = 0;

    SDL_free(path);
    return trimmedPath;
}

static void processCredentials(void** data) {
    const char* username = data[0];
    const char* password = data[1];
    bool logIn = *((bool*) data[2]);

    assert(this);
    if (!this->databaseInitialized && !databaseInit((byte*) password, NET_UNHASHED_PASSWORD_SIZE, NET_USERNAME_SIZE, NET_MESSAGE_BODY_SIZE)) { // password that's used to sign in also used to encrypt messages & other stuff in the database
        databaseClean();
        renderShowUnableToDecryptDatabaseError();
        renderHideInfiniteProgressBar();
        this->state = STATE_UNAUTHENTICATED;
        goto cleanup;
    } else
        this->databaseInitialized = true;

    this->netInitialized = netInit(
        &onMessageReceived,
        &onLogInResult,
        &onErrorReceived,
        &onRegisterResult,
        &onDisconnected,
        &logicCurrentTimeMillis,
        &onUsersFetched,
        &onConversationSetUpInviteReceived,
        &onFileExchangeInviteReceived,
        &nextFileChunkSupplier,
        &nextFileChunkReceiver
    );

    if (!this->netInitialized) {
        this->state = STATE_UNAUTHENTICATED;
        renderShowUnableToConnectToTheServerError();
        renderHideInfiniteProgressBar();
    }
    if (this->netInitialized) logIn ? netLogIn(username, password) : netRegister(username, password);

    cleanup:
    logicCredentialsRandomFiller(data[0], NET_USERNAME_SIZE);
    logicCredentialsRandomFiller(data[1], NET_UNHASHED_PASSWORD_SIZE);

    SDL_free(data[0]);
    SDL_free(data[1]);
    SDL_free(data[2]);
    SDL_free(data);
}

void logicOnCredentialsReceived(const char* username, const char* password, bool logIn) {
    assert(this);
    if (this->state != STATE_UNAUTHENTICATED) return;
    assert(!this->netInitialized);

    this->state = STATE_AWAITING_AUTHENTICATION;
    renderShowInfiniteProgressBar();

    void** data = SDL_malloc(3 * sizeof(void*));
    data[0] = SDL_malloc(NET_USERNAME_SIZE * sizeof(char));
    data[1] = SDL_malloc(NET_UNHASHED_PASSWORD_SIZE * sizeof(char));
    data[2] = SDL_malloc(sizeof(bool));

    SDL_memcpy(data[0], username, NET_USERNAME_SIZE);
    SDL_memcpy(data[1], password, NET_UNHASHED_PASSWORD_SIZE);
    *((bool*) data[2]) = logIn;

    lifecycleAsync((LifecycleAsyncActionFunction) &processCredentials, data, 0);
}

void logicCredentialsRandomFiller(char* credentials, unsigned size) {
    assert(this);
    cryptoFillWithRandomBytes((byte*) credentials, size);
}

void logicOnLoginRegisterPageQueriedByUser(bool logIn) {
    assert(this);
    logIn ? renderShowLogIn() : renderShowRegister();
}

static void startConversation(void** parameters) {
    unsigned* id = parameters[0];
    const User* user = parameters[1];

    assert(this && this->databaseInitialized);

    if (databaseConversationExists(*id))
        renderShowConversationAlreadyExists();
    else {
        Crypto* crypto = NULL;
        if ((crypto = netCreateConversation(*id)))
            databaseAddConversation(*id, crypto),
            cryptoDestroy(crypto),
            tryLoadPreviousMessages(*id),
            renderShowConversation(user->name);
        else
            renderShowUnableToCreateConversation();
    }

    SDL_free(id);
    SDL_free(parameters);

    renderHideInfiniteProgressBar();
    renderSetControlsBlocking(false);
}

static void continueConversation(void** parameters) {
    unsigned* id = parameters[0];
    const User* user = parameters[1];

    assert(this && this->databaseInitialized);

    if (!databaseConversationExists(*id))
        renderShowConversationDoesntExist();
    else
        tryLoadPreviousMessages(*id),
        renderShowConversation(user->name);

    SDL_free(id);
    SDL_free(parameters);

    renderHideInfiniteProgressBar();
    renderSetControlsBlocking(false);
}

static void createOrLoadConversation(unsigned id, bool create) {
    const User* user = findUser(id);
    assert(user);

    if (create && !(user->online)) {
        renderShowUserIsOfflineError();
        return;
    }

    renderShowInfiniteProgressBar();
    renderSetControlsBlocking(true);

    void** parameters = SDL_malloc(2 * sizeof(void*));
    (parameters[0] = SDL_malloc(sizeof(int))) && (*((unsigned*) parameters[0]) = id);
    ((const void**) parameters)[1] = user;

    lifecycleAsync((LifecycleAsyncActionFunction) (create ? &startConversation : &continueConversation), parameters, 0); // TODO: update users list after start/continue is performed
}

static void deleteConversation(unsigned* id) { // TODO: remove all messages belonging to this conversation as well
    assert(this && this->databaseInitialized); // TODO: test the following situation: A sends invite to B, and while invite is still pending acceptation, C sends an invite to A or B

    if (databaseConversationExists(*id)) databaseRemoveConversation(*id);
    else renderShowConversationDoesntExist();

    SDL_free(id);

    renderHideInfiniteProgressBar();
    renderSetControlsBlocking(false);
    logicOnUpdateUsersListClicked();
}

void logicOnUserForConversationChosen(unsigned id, RenderConversationChooseVariants chooseVariant) {
    assert(this);
    this->state = STATE_EXCHANGING_MESSAGES;
    this->toUserId = id;
    listClear(this->messagesList);

    switch (chooseVariant) {
        case RENDER_START_CONVERSATION:
        case RENDER_CONTINUE_CONVERSATION:
            createOrLoadConversation(id, chooseVariant == RENDER_START_CONVERSATION);
            break;
        case RENDER_DELETE_CONVERSATION:
            renderShowInfiniteProgressBar();
            renderSetControlsBlocking(true);

            unsigned* xId = SDL_malloc(sizeof(int));
            *xId = id;
            lifecycleAsync((LifecycleAsyncActionFunction) &deleteConversation, xId, 0);
            break;
        default:
            assert(false);
    }
}

void logicOnServerShutdownRequested(void) {
    assert(this);
    if (!this->netInitialized) return;

    renderShowInfiniteProgressBar();
    lifecycleAsync((LifecycleAsyncActionFunction) &netShutdownServer, NULL, 0);
}

void logicOnReturnFromConversationPageRequested(void) {
    assert(this && this->databaseInitialized);
    renderShowUsersList(this->currentUserName);
}

static void utoa(char* buffer, unsigned length, unsigned number) { // unsigned to ascii (string)
    byte* digits = NULL;
    unsigned digitsSize = 0;

    while (number > 0) {
        digits = SDL_realloc(digits, ++digitsSize * sizeof(char));
        digits[digitsSize - 1] = number % 10;
        number /= 10;
    }

    assert(length >= digitsSize);

    for (unsigned i = digitsSize, j = 0; i < length; i++, j++)
        buffer[j] = '0' + 0;
    for (unsigned i = length - digitsSize, j = digitsSize - 1; i < length; i++, j--)
        buffer[i] = (char) ('0' + digits[j]);

    SDL_free(digits);
}

static void monthToName(char* buffer, unsigned monthIndex) {
    assert(monthIndex < 12);
    const char months[12][3] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    SDL_memcpy(buffer, months[monthIndex], 3);
}

char* logicMillisToDateTime(unsigned long millis) {
    millis /= 1000;
    struct tm* tm = localtime((long*) &millis);
    assert(tm);

    // 'hh:mm:ss mmm-dd-yyyy'  20 + \0 = 21
    char* text = SDL_calloc(21, sizeof(char));

    utoa(text, 2, tm->tm_hour);
    text[2] = ':';
    utoa(text + 3, 2, tm->tm_min);
    text[5] = ':';
    utoa(text + 6, 2, tm->tm_sec);
    text[8] = ' ';

    monthToName(text + 9, tm->tm_mon);
    text[12] = '-';
    utoa(text + 13, 2, tm->tm_mday);
    text[15] = '-';
    utoa(text + 16, 4, tm->tm_year + 1900);
    text[20] = 0;

    return text;
}

unsigned long logicCurrentTimeMillis(void) {
    struct timespec timespec;
    assert(!clock_gettime(CLOCK_REALTIME, &timespec));
    return timespec.tv_sec * (unsigned) 1e3f + timespec.tv_nsec / (unsigned) 1e6f;
}

static void sendMessage(void** params) { // TODO: block controls and show inf progress bar while sending/receiving a message
    assert(this && params && this->databaseInitialized);

    unsigned size = *((unsigned*) params[1]), encryptedSize = cryptoEncryptedSize(size);
    assert(size && encryptedSize <= NET_MESSAGE_BODY_SIZE);

    const byte* text = params[0];
    DatabaseMessage* dbMessage = databaseMessageCreate(logicCurrentTimeMillis(), this->toUserId, netCurrentUserId(), text, size);
    databaseAddMessage(dbMessage);
    databaseMessageDestroy(dbMessage);

    Crypto* crypto = databaseGetConversation(this->toUserId);
    assert(crypto);
    byte* encryptedText = cryptoEncrypt(crypto, text, size, false);
    cryptoDestroy(crypto);

    byte body[NET_MESSAGE_BODY_SIZE];
    SDL_memset(body, 0, NET_MESSAGE_BODY_SIZE);
    SDL_memcpy(body, encryptedText, encryptedSize);
    SDL_free(encryptedText);

    netSend(NET_FLAG_PROCEED, body, encryptedSize, this->toUserId);

    SDL_free(params[0]);
    SDL_free(params[1]);
    SDL_free(params);

    renderSetControlsBlocking(false);
    renderHideInfiniteProgressBar();
}

void logicOnSendClicked(const char* text, unsigned size) {
    assert(this);
    if (!size) return;

    renderSetControlsBlocking(true);
    renderShowInfiniteProgressBar();

    void** params = SDL_malloc(2 * sizeof(void*));

    params[0] = SDL_malloc(size * sizeof(char));
    SDL_memcpy(params[0], text, size);

    params[1] = SDL_malloc(sizeof(int));
    *((unsigned*) params[1]) = size;

    listAddFront(this->messagesList, conversationMessageCreate(logicCurrentTimeMillis(), NULL, 0, text, size));
    lifecycleAsync((LifecycleAsyncActionFunction) &sendMessage, params, 0);
}

void logicOnUpdateUsersListClicked(void) {
    assert(this);

    renderShowInfiniteProgressBar();
    renderSetControlsBlocking(true);

    listClear(this->usersList);
    renderShowUsersList(this->currentUserName);

    lifecycleAsync((LifecycleAsyncActionFunction) &netFetchUsers, NULL, 0);
}

unsigned logicUnencryptedMessageBodySize(void) { return NET_MESSAGE_BODY_SIZE - cryptoEncryptedSize(0); } // 928 - 17 = 911

void logicClean(void) {
    assert(this);

    if (this->databaseInitialized) databaseClean();

    SDL_free(this->currentUserName);

    listDestroy(this->usersList);
    listDestroy(this->messagesList);

    SDL_free(this->executablePath);

    if (this->netInitialized) netClean();
    SDL_free(this);
}
