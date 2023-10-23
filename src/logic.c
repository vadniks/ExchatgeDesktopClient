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

#include <SDL.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "render/render.h"
#include "crypto.h"
#include "net.h"
#include "user.h"
#include "conversationMessage.h"
#include "lifecycle.h"
#include "database/database.h"
#include "options.h"
#include "logic.h"

const unsigned LOGIC_MAX_FILE_PATH_SIZE = 0x1ff; // 511, (1 << 9) - 1

STATIC_CONST_UNSIGNED MAX_FILE_SIZE = (1 << 20) * 20; // 1024^2 * 20 = 20971520 bytes = 20 mb

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
    bool adminMode;
    atomic unsigned state;
    char* currentUserName;
    atomic unsigned toUserId; // the id of the user, the current user (logged in via this client) wanna speak to
    atomic bool databaseInitialized;
    SDL_RWops* nullable rwops;
    atomic unsigned fileBytesCounter;
    bool autoLoggingIn;
)
#pragma clang diagnostic pop

static long fetchHostId(void);

static void showLoginPageOrPerformAutoLoggingIn(void) {
    if (!this->autoLoggingIn) {
        renderShowLogIn();
        return;
    }

    const char* credentials = optionsCredentials();
    assert(credentials);

    logicOnCredentialsReceived(
        credentials,
        NET_USERNAME_SIZE,
        credentials + NET_USERNAME_SIZE,
        NET_UNHASHED_PASSWORD_SIZE,
        true
    );
}

void logicInit(void) {
    assert(!this);
    this = SDL_malloc(sizeof *this);
    this->netInitialized = false;
    this->usersList = userInitList();
    this->messagesList = conversationMessageInitList();
    this->state = STATE_UNAUTHENTICATED;
    this->currentUserName = SDL_calloc(NET_USERNAME_SIZE, sizeof(char));
    this->databaseInitialized = false;
    this->rwops = NULL;

    assert(optionsInit(NET_USERNAME_SIZE, NET_UNHASHED_PASSWORD_SIZE, &fetchHostId));
    this->adminMode = optionsIsAdmin();
    this->autoLoggingIn = optionsCredentials() != NULL;

    lifecycleAsync((LifecycleAsyncActionFunction) &showLoginPageOrPerformAutoLoggingIn, NULL, 1000);
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

List* logicUsersList(void) {
    assert(this);
    return this->usersList;
}

List* logicMessagesList(void) {
    assert(this);
    return this->messagesList;
}

void logicOnAutoLoggingInChanged(bool value) {
    assert(this);

    if (this->autoLoggingIn)
        optionsSetCredentials(NULL);

    this->autoLoggingIn = value;
}

bool logicAutoLoggingInSupplier(void) {
    assert(this);
    return this->autoLoggingIn;
}

static int findUserComparator(const unsigned* xId, const User* const* user)
{ return *xId < (*user)->id ? -1 : (*xId > (*user)->id ? 1 : 0); }

static const User* nullable findUser(unsigned id)
{ return listBinarySearch(this->usersList, &id, (ListComparator) &findUserComparator); } // TODO: add mutex to Crypto objects

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
    assert(databaseAddMessage(dbMessage));
    databaseMessageDestroy(dbMessage);

    if (fromId == this->toUserId)
        listAddFront(this->messagesList, conversationMessageCreate(timestamp, user->name, NET_USERNAME_SIZE, (const char*) message, size));
    SDL_free(message); // TODO: limit the messages count per conversation as the encryption is safe only for $(int32.MAX) (> 2147000000) messages
}

static void onLogInResult(bool successful) { // TODO: add broadcasting to all users feature for admin
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
    renderHideInfiniteProgressBar(); // TODO: apply these only for logging in/registration
    renderSetControlsBlocking(false);
    renderShowSystemError();
}

static void onRegisterResult(bool successful) {
    assert(this);
    this->state = STATE_UNAUTHENTICATED;
    successful ? renderShowRegistrationSucceededSystemMessage() : renderShowSystemError();
    renderHideInfiniteProgressBar();
}

static void onDisconnected(void) {
    assert(this);

    this->netInitialized = false;
    this->state = STATE_UNAUTHENTICATED;

    renderHideInfiniteProgressBar();
    renderSetControlsBlocking(false);
    renderShowLogIn();
    renderShowDisconnectedError();
}

static void onUsersFetched(NetUserInfo** infos, unsigned size) {
    assert(this && this->databaseInitialized);
    listClear(this->usersList);

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
        databaseRemoveConversation(xFromId), // existence of a conversation when receiving an invite means that user, from whom this invite came, has deleted conversation with the current user, so to make users messaging again, remove the outdated conversation on the current user's side
        databaseRemoveMessages(xFromId);

    const User* user = findUser(xFromId);
    if (!user) goto releaseLocks; // if local users list hasn't been synchronized yet

    Crypto* crypto = netReplyToPendingConversationSetUpInvite(renderShowInviteDialog(user->name), xFromId);
    if (crypto)
        this->toUserId = xFromId, // not only in python there's indentation based scoping, here's an emulation though
        this->state = STATE_EXCHANGING_MESSAGES,

        assert(databaseAddConversation(xFromId, crypto)),
        cryptoDestroy(crypto),

        tryLoadPreviousMessages(xFromId),
        renderShowConversation(user->name);
    else
        renderShowUnableToCreateConversation();

    releaseLocks:
    renderSetControlsBlocking(false);
    renderHideInfiniteProgressBar();
}

static void onConversationSetUpInviteReceived(unsigned fromId) {
    assert(this);

    renderShowInfiniteProgressBar();
    renderSetControlsBlocking(true);

    unsigned* xFromId = SDL_malloc(sizeof(int));
    *xFromId = fromId;
    lifecycleAsync((LifecycleAsyncActionFunction) &replyToConversationSetUpInvite, xFromId, 0);
}

void logicOnFileChooserRequested(void) {
    assert(this);
    renderShowFileChooser();
}

static void beginFileExchange(unsigned* fileSize) {
    assert(this);
    this->fileBytesCounter = 0;

    if (!netBeginFileExchange(this->toUserId, *fileSize) || this->fileBytesCounter != *fileSize) { // blocks the thread until file is fully transmitted or error occurred or receiver declined the exchanging
        assert(!SDL_RWclose(this->rwops));
        this->rwops = NULL;

        renderShowUnableToTransmitFileError();
    } else
        renderShowFileTransmittedSystemMessage();

    SDL_free(fileSize);

    renderHideInfiniteProgressBar();
    renderSetControlsBlocking(false);
}

void logicFileChooseResultHandler(const char* nullable filePath, unsigned size) {
    assert(this);
    assert(filePath || !filePath && !size);

    renderShowInfiniteProgressBar();
    renderSetControlsBlocking(true);

    const User* user = findUser(this->toUserId);
    assert(user);

    if (!filePath) {
        renderHideInfiniteProgressBar();
        renderSetControlsBlocking(false);

        renderShowConversation(user->name);
        return;
    }

    if (!user->online) {
        renderHideInfiniteProgressBar();
        renderSetControlsBlocking(false);

        renderShowUserIsOfflineError();

        return;
    }

    if (!size) {
        renderHideInfiniteProgressBar();
        renderSetControlsBlocking(false);

        renderShowEmptyFilePathError();
        return;
    }

    assert(!this->rwops);
    this->rwops = SDL_RWFromFile(filePath, "rb");

    if (!this->rwops) {
        renderHideInfiniteProgressBar();
        renderSetControlsBlocking(false);

        renderShowCannotOpenFileError();
        return;
    }

    const long fileSize = SDL_RWsize(this->rwops);

    if (fileSize < 0 || !fileSize) {
        assert(!SDL_RWclose(this->rwops));
        this->rwops = NULL;

        renderHideInfiniteProgressBar();
        renderSetControlsBlocking(false);

        renderShowFileIsEmptyError();
        return;
    }

    if (fileSize > MAX_FILE_SIZE) {
        assert(!SDL_RWclose(this->rwops));
        this->rwops = NULL;

        renderHideInfiniteProgressBar();
        renderSetControlsBlocking(false);

        renderShowFileIsTooBig();
        return;
    }

    unsigned* xFileSize = SDL_malloc(sizeof(int));
    *xFileSize = (unsigned) fileSize;
    lifecycleAsync((LifecycleAsyncActionFunction) &beginFileExchange, xFileSize, 0);
}

static void replyToFileExchangeRequest(unsigned** parameters) {
    assert(this);
    this->fileBytesCounter = 0;

    const unsigned fromId = *(parameters[0]), fileSize = *(parameters[1]);
    SDL_free(parameters[0]), SDL_free(parameters[1]), SDL_free(parameters);

    const User* user = findUser(fromId);
    assert(user);

    const bool accepted = renderShowFileExchangeRequestDialog(user->name, fileSize); // blocks the thread
    assert(this);

    if (!accepted) {
        assert(!netReplyToFileExchangeInvite(fromId, fileSize, false)); // blocks the thread again

        renderHideInfiniteProgressBar();
        renderSetControlsBlocking(false);

        return;
    }

    char currentPath[LOGIC_MAX_FILE_PATH_SIZE];
    assert(getcwd(currentPath, LOGIC_MAX_FILE_PATH_SIZE));

    char filePath[LOGIC_MAX_FILE_PATH_SIZE];
    const unsigned written = SDL_snprintf(
        filePath, LOGIC_MAX_FILE_PATH_SIZE,
        "%s/%lu",
        currentPath,
        logicCurrentTimeMillis()
    );
    assert(written > 0 && written <= LOGIC_MAX_FILE_PATH_SIZE);

    assert(!this->rwops);
    this->rwops = SDL_RWFromFile(filePath, "wb");

    if (!this->rwops) {
        assert(!netReplyToFileExchangeInvite(fromId, fileSize, false));

        renderHideInfiniteProgressBar();
        renderSetControlsBlocking(false);

        renderShowUnableToTransmitFileError();
        return;
    }

    if (!netReplyToFileExchangeInvite(fromId, fileSize, true) || this->fileBytesCounter != fileSize) { // blocks the thread again
        renderShowUnableToTransmitFileError();
        assert(!SDL_RWclose(this->rwops));
        this->rwops = NULL;
    } else
        renderShowFileTransmittedSystemMessage();

    assert(!this->rwops);
    renderHideInfiniteProgressBar();
    renderSetControlsBlocking(false);
}

static void onFileExchangeInviteReceived(unsigned fromId, unsigned fileSize) {
    assert(this);

    renderShowInfiniteProgressBar();
    renderSetControlsBlocking(true);

    unsigned** parameters = SDL_malloc(2 * sizeof(void*));
    (parameters[0] = SDL_malloc(sizeof(int))) && (*(parameters[0]) = fromId);
    (parameters[1] = SDL_malloc(sizeof(int))) && (*(parameters[1]) = fileSize);

    lifecycleAsync((LifecycleAsyncActionFunction) &replyToFileExchangeRequest, parameters, 0);
}

static unsigned nextFileChunkSupplier(unsigned index, byte* encryptedBuffer) { // TODO: notify user when a new message has been received
    assert(this && this->rwops);
    const unsigned targetSize = logicUnencryptedMessageBodySize();

    byte unencryptedBuffer[targetSize];
    const unsigned actualSize = SDL_RWread(this->rwops, unencryptedBuffer, 1, targetSize);
    if (!actualSize) goto closeFile;

    Crypto* crypto = databaseGetConversation(this->toUserId);
    assert(crypto);

    const unsigned encryptedSize = cryptoEncryptedSize(actualSize);
    assert(encryptedSize <= NET_MESSAGE_BODY_SIZE);

    byte* encryptedChunk = cryptoEncrypt(crypto, unencryptedBuffer, actualSize, false);
    SDL_memcpy(encryptedBuffer, encryptedChunk, encryptedSize);
    SDL_free(encryptedChunk);

    cryptoDestroy(crypto);

    if (!index) assert(!this->fileBytesCounter);
    this->fileBytesCounter += actualSize;
    return encryptedSize;

    closeFile:
    assert(index);
    assert(!SDL_RWclose(this->rwops));
    this->rwops = NULL;
    return 0;
}

static void nextFileChunkReceiver(unsigned fromId, unsigned index, unsigned fileSize, unsigned receivedBytesCount, const byte* encryptedBuffer) {
    assert(this && this->rwops);

    const unsigned decryptedSize = receivedBytesCount - cryptoEncryptedSize(0);
    assert(decryptedSize && decryptedSize <= logicUnencryptedMessageBodySize());

    Crypto* crypto = databaseGetConversation(fromId);
    assert(crypto);

    byte* decrypted = cryptoDecrypt(crypto, encryptedBuffer, receivedBytesCount, false);
    assert(decrypted);

    assert(SDL_RWwrite(this->rwops, decrypted, 1, decryptedSize) == decryptedSize);
    SDL_free(decrypted);

    cryptoDestroy(crypto);

    if (!index) assert(!this->fileBytesCounter);
    this->fileBytesCounter += decryptedSize;

    if (this->fileBytesCounter < fileSize) return;

    assert(!SDL_RWclose(this->rwops));
    this->rwops = NULL;
}

static void clipboardPaste(void) {
    if (!SDL_HasClipboardText()) return;
    if (!renderIsConversationShown() && !renderIsFileChooserShown()) return;

    const unsigned maxSize = renderIsConversationShown() ? NET_MESSAGE_BODY_SIZE : LOGIC_MAX_FILE_PATH_SIZE;
    char* text = SDL_GetClipboardText(), * buffer = NULL;
    unsigned size;

    for (size = 0; text[size] && size <= maxSize; size++)
        buffer = SDL_realloc(buffer, size + 1),
        buffer[size] = text[size];

    SDL_free(text);

    if (size) {
        const unsigned totalSize = size <= maxSize ? size : maxSize;
        renderIsConversationShown()
            ? renderAlterConversationMessageBuffer(buffer, totalSize)
            : renderAlterFilePathBuffer(buffer, totalSize);
    }
    SDL_free(buffer);
}

static void clipboardCopy(void) {}

void logicProcessEvent(SDL_Event* event) {
    static bool ctrlPressed;
    if (event->key.keysym.sym == SDLK_LCTRL)
        ctrlPressed = event->type == SDL_KEYDOWN;

    const SDL_KeyCode keyCode = event->key.keysym.sym;

    if (ctrlPressed && event->type == SDL_KEYUP)
         keyCode == SDLK_v
            ? clipboardPaste()
            : keyCode == SDLK_c
                ? clipboardCopy()
                : STUB;
} // TODO: when exchanging file add support for file names and perform checksum

// TODO: add possibility for admin to remove users from database; to disable/enable registration; to ban/unban users; to kick connected users

static void processCredentials(void** data) {
    const char* username = data[0];
    const char* password = data[1];
    const bool logIn = *((bool*) data[2]);

    assert(this);
    if (!logIn) goto netInit;

    if (this->autoLoggingIn) {
        char credentials[NET_USERNAME_SIZE + NET_UNHASHED_PASSWORD_SIZE];
        SDL_memcpy(credentials, username, NET_USERNAME_SIZE);
        SDL_memcpy(credentials + NET_USERNAME_SIZE, password, NET_UNHASHED_PASSWORD_SIZE);
        optionsSetCredentials(credentials);
        logicCredentialsRandomFiller(credentials, sizeof credentials);
    }

    if (this->databaseInitialized) {
        databaseClean();
        this->databaseInitialized = false;
    }

    if (!databaseInit(
        (byte*) password,
        NET_UNHASHED_PASSWORD_SIZE,
        NET_USERNAME_SIZE,
        NET_MESSAGE_BODY_SIZE,
        &fetchHostId
    )) { // password that's used to sign in also used to encrypt messages & other stuff in the database
        databaseClean();
        renderShowUnableToDecryptDatabaseError();
        renderHideInfiniteProgressBar();
        this->state = STATE_UNAUTHENTICATED;

        if (this->autoLoggingIn)
            renderShowLogIn();

        goto cleanup;
    } else
        this->databaseInitialized = true;

    netInit:
    this->netInitialized = netInit(
        optionsHost(),
        optionsPort(),
        optionsServerSignPublicKey(),
        optionsServerSignPublicKeySize(),
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

        if (this->autoLoggingIn)
            renderShowLogIn();
    } else
        logIn ? netLogIn(username, password) : netRegister(username, password);

    cleanup:
    logicCredentialsRandomFiller(data[0], NET_USERNAME_SIZE);
    logicCredentialsRandomFiller(data[1], NET_UNHASHED_PASSWORD_SIZE);

    SDL_free(data[0]);
    SDL_free(data[1]);
    SDL_free(data[2]);
    SDL_free(data);
}

void logicOnCredentialsReceived(
    const char* username,
    unsigned usernameSize,
    const char* password,
    unsigned passwordSize,
    bool logIn
) {
    assert(this && usernameSize <= NET_USERNAME_SIZE && passwordSize <= NET_UNHASHED_PASSWORD_SIZE);
    if (this->state != STATE_UNAUTHENTICATED) return;
    assert(!this->netInitialized);

    this->state = STATE_AWAITING_AUTHENTICATION;
    renderShowInfiniteProgressBar();

    void** data = SDL_malloc(3 * sizeof(void*));
    data[0] = SDL_calloc(NET_USERNAME_SIZE, sizeof(char));
    data[1] = SDL_calloc(NET_UNHASHED_PASSWORD_SIZE, sizeof(char));
    data[2] = SDL_malloc(sizeof(bool));

    SDL_memcpy(data[0], username, usernameSize);
    SDL_memcpy(data[1], password, passwordSize);
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
        if ((crypto = netCreateConversation(*id))) // blocks the thread until either an error has happened or the conversation has been created
            assert(databaseAddConversation(*id, crypto)),
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

static void deleteConversation(unsigned* id) {
    assert(this && this->databaseInitialized);

    if (databaseConversationExists(*id)) {
        databaseRemoveConversation(*id);
        databaseRemoveMessages(*id);
    } else
        renderShowConversationDoesntExist();

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
    renderSetControlsBlocking(true);
    lifecycleAsync((LifecycleAsyncActionFunction) &netShutdownServer, NULL, 0);
}

void logicOnReturnFromConversationPageRequested(void) {
    assert(this && this->databaseInitialized);
    renderShowUsersList(this->currentUserName);
}

static void utoa(char* buffer, unsigned length, unsigned number) { // unsigned to ascii (string) (and to utf-8 too as all the english letters are presented in utf-8 the same way as they're in ascii)
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

static void sendMessage(void** params) {
    assert(this && params && this->databaseInitialized);

    unsigned size = *((unsigned*) params[1]), encryptedSize = cryptoEncryptedSize(size);
    assert(size && encryptedSize <= NET_MESSAGE_BODY_SIZE);

    const byte* text = params[0];
    DatabaseMessage* dbMessage = databaseMessageCreate(logicCurrentTimeMillis(), this->toUserId, netCurrentUserId(), text, size);
    assert(databaseAddMessage(dbMessage));
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
}

void logicOnSendClicked(const char* text, unsigned size) {
    assert(this);
    if (!size) return;

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

static long fetchHostId(void) {
    const long dummy = (long) 0xfffffffffffffffful;

    SDL_RWops* rwOps = SDL_RWFromFile("/etc/machine-id", "rb");
    if (!rwOps) return dummy;

    const byte size = 1 << 5; // 32
    byte buffer[size];

    const bool sizeMatched = SDL_RWread(rwOps, buffer, 1, size) == size;
    assert(!SDL_RWclose(rwOps));
    if (!sizeMatched) return dummy;

    long hash = 0;
    for (byte i = 0; i < size; hash = 63 * hash + buffer[i++]);
    return hash;
}

void logicClean(void) {
    assert(this);

    assert(!this->rwops);

    if (this->databaseInitialized) databaseClean();

    optionsClear();

    SDL_free(this->currentUserName);

    listDestroy(this->usersList);
    listDestroy(this->messagesList);

    if (this->netInitialized) netClean();
    SDL_free(this);
}
