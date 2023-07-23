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

#include <assert.h>
//#include "nuklearDefs.h" - cmake automatically includes precompiled version of this header
#include "../defs.h"
#include "../collections/queue.h"
#include "../user.h"
#include "../conversationMessage.h"
#include "render.h"

#define SYNCHRONIZED_BEGIN assert(!SDL_LockMutex(this->uiQueriesMutex)); {
#define SYNCHRONIZED_END assert(!SDL_UnlockMutex(this->uiQueriesMutex)); }
#define SYNCHRONIZED(x) SYNCHRONIZED_BEGIN x SYNCHRONIZED_END

STATIC_CONST_UNSIGNED WINDOW_WIDTH = 16 * 50;
STATIC_CONST_UNSIGNED WINDOW_HEIGHT = 9 * 50;
STATIC_CONST_UNSIGNED FONT_SIZE = 14;
STATIC_CONST_UNSIGNED MAX_U32_DEC_DIGITS_COUNT = 10; // 0xffffffff = 4294967295 10 digits

typedef enum : unsigned {
    STATE_INITIAL = 0,
    STATE_LOG_IN = 1,
    STATE_REGISTER = 2,
    STATE_USERS_LIST = 3,
    STATE_CONVERSATION = 4
} States;

STATIC_CONST_STRING TITLE = "Exchatge";
STATIC_CONST_STRING SUBTITLE = "A secured text exchanger";
STATIC_CONST_STRING LOG_IN = "Log in";
STATIC_CONST_STRING REGISTER = "Register";
STATIC_CONST_STRING USERNAME = "Username";
STATIC_CONST_STRING PASSWORD = "Password";
STATIC_CONST_STRING PROCEED = "Proceed";
STATIC_CONST_STRING USERS_LIST = "Users list";
STATIC_CONST_STRING START_CONVERSATION = "Start conversation";
STATIC_CONST_STRING CONTINUE_CONVERSATION = "Continue conversation";
STATIC_CONST_STRING DELETE_CONVERSATION = "Delete conversation";
STATIC_CONST_STRING ID_TEXT = "Id";
STATIC_CONST_STRING NAME_TEXT = "Name";
STATIC_CONST_STRING ERROR_TEXT = "Error";
STATIC_CONST_STRING WELCOME = "Welcome ";
STATIC_CONST_STRING SHUTDOWN_SERVER = "Shutdown the server";
STATIC_CONST_STRING DISCONNECTED = "Disconnected";
STATIC_CONST_STRING UNABLE_TO_CONNECT_TO_THE_SERVER = "Unable to connect to the server";
STATIC_CONST_STRING SEND = "Send";
STATIC_CONST_STRING ONLINE = "Online";
STATIC_CONST_STRING OFFLINE = "Offline";
STATIC_CONST_STRING BACK = "Back";
STATIC_CONST_STRING YOU = "You";
STATIC_CONST_STRING UPDATE = "Update";
STATIC_CONST_STRING REGISTRATION_SUCCEEDED = "Registration succeeded";
STATIC_CONST_STRING USER_IS_OFFLINE = "User is offline";
STATIC_CONST_STRING INVITATION_RECEIVED = "Invitation received";
STATIC_CONST_STRING YOU_ARE_INVITED_TO_CREATE_CONVERSATION_BY_USER = "You are invited to create conversation by user ";
STATIC_CONST_STRING ACCEPT = "Accept";
STATIC_CONST_STRING DECLINE = "Decline";
STATIC_CONST_STRING UNABLE_TO_DECRYPT_DATABASE = "Unable to decrypt the database";
STATIC_CONST_STRING UNABLE_TO_CREATE_CONVERSATION = "Unable to create the conversation";
STATIC_CONST_STRING CONVERSATION_DOESNT_EXIST = "Conversation doesn't exist";
STATIC_CONST_STRING CONVERSATION_ALREADY_EXISTS = "Conversation already exists";

const unsigned RENDER_MAX_MESSAGE_SYSTEM_TEXT_SIZE = 64;

typedef struct {
    char text[RENDER_MAX_MESSAGE_SYSTEM_TEXT_SIZE]; // TODO: rename constant
    bool error;
} SystemMessage;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    unsigned width;
    unsigned height;
    SDL_Window* window;
    SDL_Renderer* renderer;
    struct nk_context* context;
    struct nk_colorf colorf;
    States state;
    unsigned usernameSize;
    unsigned passwordSize;
    RenderCredentialsReceivedCallback onCredentialsReceived;
    RenderCredentialsRandomFiller credentialsRandomFiller;
    unsigned enteredUsernameSize;
    unsigned enteredPasswordSize;
    char* enteredCredentialsBuffer;
    RenderLogInRegisterPageQueriedByUserCallback onLoginRegisterPageQueriedByUser;
    SDL_mutex* uiQueriesMutex;
    const List* usersList; // <User*> allocated elsewhere
    RenderUserForConversationChosenCallback onUserForConversationChosen;
    const List* conversationMessagesList; // <ConversationMessage*> allocated elsewhere, conversation messages
    unsigned maxMessageSize; // conversation message size // TODO: rename to conversationMessageSize
    char* conversationName; // conversation name or the name of the recipient
    unsigned conversationNameSize;
    bool adminMode;
    RenderOnServerShutdownRequested onServerShutdownRequested;
    char* conversationMessage; // conversation message buffer for current user
    unsigned enteredConversationMessageSize;
    bool loading;
    char infiniteProgressBarAnim;
    unsigned infiniteProgressBarCounter;
    Queue* systemMessagesQueue;
    SystemMessage* nullable currentSystemMessage;
    unsigned systemMessageTicks;
    RenderOnReturnFromConversationPageRequested onReturnFromConversationPageRequested;
    RenderMillisToDateTimeConverter millisToDateTimeConverter;
    RenderOnSendClicked onSendClicked;
    RenderOnUpdateUsersListClicked onUpdateUsersListClicked;
    char* currentUserName; // the name of the user who is currently logged in this client
    bool allowInput;
)
#pragma clang diagnostic pop

static void setStyle(void) {
    struct nk_color table[NK_COLOR_COUNT];
    table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
    table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
    table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
    table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
    table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
    table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
    table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
    table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
    table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
    table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
    table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
    table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
    table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
    table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
    table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
    nk_style_from_table(this->context, table);
}

static void destroySystemMessage(SystemMessage* message) { SDL_free(message); }

void renderInit(
    unsigned usernameSize,
    unsigned passwordSize,
    RenderCredentialsReceivedCallback onCredentialsReceived,
    RenderCredentialsRandomFiller credentialsRandomFiller,
    RenderLogInRegisterPageQueriedByUserCallback onLoginRegisterPageQueriedByUser,
    RenderUserForConversationChosenCallback onUserForConversationChosen,
    unsigned conversationNameSize,
    RenderOnServerShutdownRequested onServerShutdownRequested,
    RenderOnReturnFromConversationPageRequested onReturnFromConversationPageRequested,
    RenderMillisToDateTimeConverter millisToDateTimeConverter,
    RenderOnSendClicked onSendClicked,
    RenderOnUpdateUsersListClicked onUpdateUsersListClicked
) {
    assert(!this);
    this = SDL_malloc(sizeof *this);
    this->width = WINDOW_WIDTH;
    this->height = WINDOW_HEIGHT;
    this->state = STATE_INITIAL;

    this->usernameSize = usernameSize;
    assert(usernameSize > 0);

    this->passwordSize = passwordSize;
    assert(passwordSize > 0);

    this->onCredentialsReceived = onCredentialsReceived;
    this->credentialsRandomFiller = credentialsRandomFiller;
    this->enteredUsernameSize = 0;
    this->enteredPasswordSize = 0;
    this->enteredCredentialsBuffer = SDL_calloc(this->usernameSize + this->passwordSize, sizeof(char));
    this->onLoginRegisterPageQueriedByUser = onLoginRegisterPageQueriedByUser;

    this->uiQueriesMutex = SDL_CreateMutex();
    assert(this->uiQueriesMutex);

    this->usersList = NULL;
    this->onUserForConversationChosen = onUserForConversationChosen;
    this->conversationMessagesList = NULL;
    this->conversationName = SDL_calloc(this->usernameSize, sizeof(char));

    this->conversationNameSize = conversationNameSize;
    assert(conversationNameSize > 0);

    this->adminMode = false;
    this->onServerShutdownRequested = onServerShutdownRequested;
    this->conversationMessage = SDL_calloc(this->maxMessageSize, sizeof(char));
    this->enteredConversationMessageSize = 0;
    this->loading = false;
    this->infiniteProgressBarAnim = '|';
    this->infiniteProgressBarCounter = 0;
    this->systemMessagesQueue = queueInit((QueueDeallocator) &destroySystemMessage);
    this->currentSystemMessage = NULL;
    this->systemMessageTicks = 0;
    this->onReturnFromConversationPageRequested = onReturnFromConversationPageRequested;
    this->millisToDateTimeConverter = millisToDateTimeConverter;
    this->onSendClicked = onSendClicked;
    this->onUpdateUsersListClicked = onUpdateUsersListClicked;
    this->currentUserName = SDL_calloc(this->usernameSize, sizeof(char));
    this->allowInput = true;

    this->window = SDL_CreateWindow(
        TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        (int) this->width,
        (int) this->height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE
    );
    assert(this->window);

    SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "3");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "1");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    this->renderer = SDL_CreateRenderer(this->window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    assert(this->renderer);

    int renderW, renderH, windowW, windowH;
    float scaleX, scaleY, fontScale;

    SDL_GetRendererOutputSize(this->renderer, &renderW, &renderH);
    SDL_GetWindowSize(this->window, &windowW, &windowH);

    scaleX = (float) renderW / (float) windowW;
    scaleY = (float) renderH / (float) windowH;
    SDL_SetWindowMinimumSize(this->window, (int) ((float) WINDOW_WIDTH * scaleX), (int) ((float) WINDOW_HEIGHT * scaleY));

    SDL_RenderSetScale(this->renderer, scaleX, scaleY);
    fontScale = scaleY;

    this->context = nk_sdl_init(this->window, this->renderer);

    struct nk_font_config config = nk_font_config((float) FONT_SIZE);
    config.oversample_v = STBTT_MAX_OVERSAMPLE; // 8
    config.oversample_h = STBTT_MAX_OVERSAMPLE;

    struct nk_font_atlas* atlas = NULL;
    nk_sdl_font_stash_begin(&atlas);

    struct nk_font* font = nk_font_atlas_add_default(atlas, config.size * fontScale, &config);
    nk_sdl_font_stash_end();

    font->handle.height /= fontScale;
    nk_style_set_font(this->context, &(font->handle));

    setStyle();

    this->colorf = (struct nk_colorf) { 0.10f, 0.18f, 0.24f, 1.00f };
}

void renderSetMaxMessageSize(unsigned size) {
    assert(this);
    this->maxMessageSize = size;
}

void renderSetAdminMode(bool mode) {
    assert(this);
    this->adminMode = mode;
}

void renderSetUsersList(const List* usersList) {
    assert(this);
    this->usersList = usersList;
}

void renderSetMessagesList(const List* messagesList) {
    assert(this);
    this->conversationMessagesList = messagesList;
}

void renderInputBegan(void) {
    assert(this);
    nk_input_begin(this->context);
}

void renderProcessEvent(SDL_Event* event) {
    assert(this);

    bool inputEvent = event->type == SDL_KEYDOWN
        || event->type == SDL_KEYUP
        || event->type == SDL_TEXTEDITING
        || event->type == SDL_TEXTINPUT
        || event->type == SDL_TEXTEDITING_EXT
        || event->type == SDL_MOUSEBUTTONDOWN
        || event->type == SDL_MOUSEBUTTONUP
        || event->type == SDL_MOUSEMOTION
        || event->type == SDL_MOUSEWHEEL;

    if (!this->allowInput && inputEvent) return;

    nk_sdl_handle_event(event);
}

void renderInputEnded(void) {
    assert(this);
    nk_input_end(this->context);
}

void renderSetWindowTitle(const char* title) {
    assert(this);

    const unsigned winTitleSize = SDL_strlen(TITLE), separatorSize = 2,
        size = winTitleSize + separatorSize + this->usernameSize + 1;

    char xTitle[size];
    SDL_memcpy(xTitle, TITLE, winTitleSize);
    SDL_memcpy(xTitle + winTitleSize, ", ", separatorSize);
    SDL_memcpy(xTitle + winTitleSize + separatorSize, title, this->usernameSize);
    xTitle[size - 1] = 0;

    SYNCHRONIZED(SDL_SetWindowTitle(this->window, xTitle);)
}

void renderShowLogIn(void) {
    assert(this);
    SYNCHRONIZED_BEGIN

    SDL_memset(this->enteredCredentialsBuffer, 0, this->usernameSize + this->passwordSize * sizeof(char));
    this->enteredUsernameSize = 0;
    this->enteredPasswordSize = 0;
    this->state = STATE_LOG_IN;

    SYNCHRONIZED_END
}

void renderShowRegister(void) {
    assert(this);
    SYNCHRONIZED(this->state = STATE_REGISTER;)
}

void renderShowUsersList(const char* currentUserName) {
    assert(this);
    SYNCHRONIZED_BEGIN

    SDL_memcpy(this->currentUserName, currentUserName, this->usernameSize);
    this->state = STATE_USERS_LIST;

    SYNCHRONIZED_END
}

void renderShowConversation(const char* conversationName) {
    assert(this);
    SYNCHRONIZED_BEGIN

    SDL_memcpy(this->conversationName, conversationName, this->conversationNameSize);
    this->state = STATE_CONVERSATION;

    SYNCHRONIZED_END
}

bool renderShowInviteDialog(const char* fromUserName) {
    assert(this);

    const unsigned messageSize = SDL_strlen(YOU_ARE_INVITED_TO_CREATE_CONVERSATION_BY_USER),
        xMessageSize = messageSize + 1 + this->usernameSize + 1;

    char xMessage[xMessageSize];
    SDL_memcpy(xMessage, YOU_ARE_INVITED_TO_CREATE_CONVERSATION_BY_USER, messageSize);
    xMessage[messageSize] = ' ';
    SDL_memcpy(xMessage + messageSize + 1, fromUserName, this->usernameSize);
    xMessage[xMessageSize - 1] = 0;

    const SDL_MessageBoxButtonData buttons[2] = {
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, ACCEPT },
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, DECLINE }
    };

    SDL_MessageBoxData data = {
        SDL_MESSAGEBOX_INFORMATION,
        this->window,
        INVITATION_RECEIVED,
        xMessage,
        2,
        buttons,
        NULL
    };

    int buttonId; // TODO: nk_popup_begin()
    SDL_ShowMessageBox(&data, &buttonId); // blocks the thread until user pressed any button or closed the dialog window
    return !buttonId;
}

void renderSetControlsBlocking(bool blocking) {
    assert(this);
    SYNCHRONIZED(this->allowInput = !blocking;)
}

static void postSystemMessage(const char* text, bool error) { // expects a null-terminated string with length in range (0, SYSTEM_MESSAGE_SIZE_MAX]
    SystemMessage* message = SDL_malloc(sizeof *message);
    message->error = error;

    SDL_memset(message->text, 0, RENDER_MAX_MESSAGE_SYSTEM_TEXT_SIZE);

    bool foundNullTerminator = false;
    for (unsigned i = 0; i < RENDER_MAX_MESSAGE_SYSTEM_TEXT_SIZE; i++) {
        message->text[i] = text[i];

        if (!text[i]) {
            assert(i > 0);
            foundNullTerminator = true;
            break;
        }
    }
    assert(foundNullTerminator);

    SYNCHRONIZED(queuePush(this->systemMessagesQueue, message);)
}

void renderShowSystemMessage(const char* message, bool error) {
    assert(this);
    postSystemMessage(message, error);
}

void renderHideSystemMessage(void) {
    assert(this);
    SYNCHRONIZED_BEGIN

    while (queueSize(this->systemMessagesQueue) > 0)
        destroySystemMessage(queuePop(this->systemMessagesQueue));

    SYNCHRONIZED_END
}

void renderShowSystemError(void) { postSystemMessage(ERROR_TEXT, true); }
void renderShowDisconnectedError(void) { postSystemMessage(DISCONNECTED, true); }
void renderShowUnableToConnectToTheServerError(void) { postSystemMessage(UNABLE_TO_CONNECT_TO_THE_SERVER, true); }
void renderShowRegistrationSucceededSystemMessage(void) { postSystemMessage(REGISTRATION_SUCCEEDED, false); }
void renderShowUserIsOfflineError(void) { postSystemMessage(USER_IS_OFFLINE, true); }
void renderShowUnableToDecryptDatabaseError(void) { postSystemMessage(UNABLE_TO_DECRYPT_DATABASE, true); }
void renderShowUnableToCreateConversation(void) { postSystemMessage(UNABLE_TO_CREATE_CONVERSATION, true); }
void renderShowConversationDoesntExist(void) { postSystemMessage(CONVERSATION_DOESNT_EXIST, true); }
void renderShowConversationAlreadyExists(void) { postSystemMessage(CONVERSATION_ALREADY_EXISTS, true); }

void renderShowInfiniteProgressBar(void) {
    assert(this);
    SYNCHRONIZED(this->loading = true;)
}

void renderHideInfiniteProgressBar(void) {
    assert(this);
    SYNCHRONIZED(this->loading = false;)
}

static void drawInfiniteProgressBar(float height) {
    const unsigned maxCounterValue = 60 / 12; // 5 times slower than screen update

    if (this->infiniteProgressBarCounter > 0)
        this->infiniteProgressBarCounter =
            this->infiniteProgressBarCounter < maxCounterValue ? this->infiniteProgressBarCounter + 1 : 0;
    else {
        switch (this->infiniteProgressBarAnim) { // anim: | / - \... - aka infinite circle-like progress bar
            case '|':
                this->infiniteProgressBarAnim = '/';
                break;
            case '/':
                this->infiniteProgressBarAnim = '-';
                break;
            case '-':
                this->infiniteProgressBarAnim = '\\';
                break;
            case '\\':
                this->infiniteProgressBarAnim = '|';
                break;
            default: assert(false);
        }
        (this->infiniteProgressBarCounter)++;
    }

    nk_layout_row_dynamic(this->context, height, 1);
    char animText[2] = {this->infiniteProgressBarAnim, '\0'};
    nk_label(this->context, animText, NK_TEXT_ALIGN_CENTERED);
}

static void drawSplashPage(void) {
    const float height = (float) this->height;

    nk_layout_row_dynamic(this->context, height * 0.3f, 1);
    nk_spacer(this->context);

    nk_layout_row_dynamic(this->context, height * 0.0625f, 1);
    nk_label(this->context, TITLE, NK_TEXT_CENTERED);
    nk_label(this->context, SUBTITLE, NK_TEXT_CENTERED);
    drawInfiniteProgressBar(height * 0.125f);

    nk_layout_row_dynamic(this->context, height * 0.25f, 1);
    nk_spacer(this->context);
}

static void onProceedClickedAfterLogInRegister(bool logIn) {
    (*(this->onCredentialsReceived))(
        this->enteredCredentialsBuffer,
        this->enteredCredentialsBuffer + this->usernameSize,
        logIn
    );

    (*(this->credentialsRandomFiller))(this->enteredCredentialsBuffer, this->usernameSize + this->passwordSize); // TODO: deprecated(credentialsRandomFiller)
    SDL_memset(this->enteredCredentialsBuffer, 0, this->usernameSize + this->passwordSize);
    this->enteredUsernameSize = 0;
    this->enteredPasswordSize = 0;
}

static void drawLogInForm(int width, float height, bool logIn) {
    width -= 15; height -= 25;

    char groupName[2] = {1, 0};
    if (!nk_group_begin(this->context, groupName, NK_WINDOW_NO_SCROLLBAR)) return;

    nk_layout_row_dynamic(this->context, height * 0.25f, 1);
    nk_label(this->context, logIn ? LOG_IN : REGISTER, NK_TEXT_CENTERED);

    nk_layout_row_static(this->context, height * 0.25f, width / 2, 2);

    nk_label(this->context, USERNAME, NK_TEXT_ALIGN_LEFT);
    nk_edit_string(
        this->context,
        NK_EDIT_FIELD,
        this->enteredCredentialsBuffer,
        (int*) &(this->enteredUsernameSize),
        (int) this->usernameSize,
        &nk_filter_default
    );

    nk_label(this->context, PASSWORD, NK_TEXT_ALIGN_LEFT);
    nk_edit_string(
        this->context,
        NK_EDIT_FIELD,
        this->enteredCredentialsBuffer + this->usernameSize,
        (int*) &(this->enteredPasswordSize),
        (int) this->passwordSize,
        &nk_filter_default
    );

    nk_layout_row_static(this->context, height * 0.25f, width / 2, 2);
    if (nk_button_label(this->context, PROCEED)) onProceedClickedAfterLogInRegister(logIn);
    if (nk_button_label(this->context, logIn ? REGISTER : LOG_IN)) (*(this->onLoginRegisterPageQueriedByUser))(!logIn);

    nk_group_end(this->context);
}

static unsigned decreaseWidthIfNeeded(unsigned width) { return width <= WINDOW_WIDTH ? width : WINDOW_WIDTH; }
static unsigned decreaseHeightIfNeeded(unsigned height) { return height <= WINDOW_HEIGHT ? height : WINDOW_HEIGHT; }
static float currentHeight(void) { return (float) this->height * 0.925f; }

static void drawLoginPage(bool logIn) {
    const float height = currentHeight();

    nk_layout_row_dynamic(this->context, height * 0.25f, 1);
    nk_spacer(this->context);

    const float width = (float) this->width;

    float height2 = min(width, decreaseHeightIfNeeded(height)) * 0.4f;
    if (this->loading) height2 -= 0.05f * height;
    if (this->currentSystemMessage) height2 -= 0.05f * height;

    nk_layout_row_begin(this->context, NK_STATIC, height2, 3);

    nk_layout_row_push(this->context, width * 0.25f);
    nk_spacer(this->context);

    const float width2 = width * 0.925f;
    nk_layout_row_push(this->context, width2 * 0.5f);
    drawLogInForm((int) (width2 * 0.5f), height2, logIn);

    nk_layout_row_push(this->context, width * 0.25f);
    nk_spacer(this->context);

    nk_layout_row_end(this->context);
}

static void drawUserRowColumn(
    byte groupId,
    float ration,
    unsigned id,
    float height2,
    void (*contentDrawer)(const void* nullable),
    const void* nullable parameter
) {
    const unsigned intSize = sizeof(int);

    char groupName[2 + intSize];
    groupName[0] = (char) groupId;
    SDL_memcpy(groupName + 1, &id, intSize);
    groupName[1 + intSize] = 0;

    nk_layout_row_push(this->context, ration);
    if (nk_group_begin(this->context, groupName, NK_WINDOW_NO_SCROLLBAR)) {

        nk_layout_row_dynamic(this->context, height2, 1);
        (*contentDrawer)(parameter);
        nk_group_end(this->context);
    }
}

static void drawUserRowColumnDescriptions(__attribute_maybe_unused__ const void* nullable parameter) {
    nk_label(this->context, ID_TEXT, NK_TEXT_ALIGN_LEFT);
    nk_label(this->context, NAME_TEXT, NK_TEXT_ALIGN_LEFT);
}

static void drawUserRowColumnIdAndName(const void* parameter) {
    assert(parameter);
    nk_label(this->context, /*idString*/ (const char*) ((const void**) parameter)[0], NK_TEXT_ALIGN_LEFT);
    nk_label(this->context, /*name*/ (const char*) ((const void**) parameter)[1], NK_TEXT_ALIGN_LEFT);
}

static void drawUserRowColumnStatus(const void* parameter) {
    assert(parameter);
    nk_label(this->context, /*online*/ *((const bool*) parameter) ? ONLINE : OFFLINE, NK_TEXT_ALIGN_LEFT);
}

static void drawUserRowColumnActions(const void* parameter) {
    assert(parameter);
    const unsigned id = *((const unsigned*) ((const void**) parameter)[0]);

    if (/*conversationExists*/ *((const bool*) ((const void**) parameter)[1])) {
        if (nk_button_label(this->context, CONTINUE_CONVERSATION))
            (*(this->onUserForConversationChosen))(id, RENDER_CONTINUE_CONVERSATION);

        if (nk_button_label(this->context, DELETE_CONVERSATION))
            (*(this->onUserForConversationChosen))(id, RENDER_DELETE_CONVERSATION);
    } else {
        if (nk_button_label(this->context, START_CONVERSATION))
            (*(this->onUserForConversationChosen))(id, RENDER_START_CONVERSATION);

        nk_spacer(this->context);
    }
}

static void drawUserRow(unsigned id, const char* idString, const char* name, bool conversationExists, bool online) {
    const float height = (float) decreaseHeightIfNeeded(this->height) * 0.925f * 0.15f, height2 = height * 0.5f * 0.7f;
    nk_layout_row_begin(this->context, NK_DYNAMIC, height, 4);

    drawUserRowColumn(1, 0.1f, id, height2, &drawUserRowColumnDescriptions, NULL);
    drawUserRowColumn(2, 0.5f, id, height2, &drawUserRowColumnIdAndName, (const void*[2]) {idString, name});
    drawUserRowColumn(3, 0.1f, id, height2, &drawUserRowColumnStatus, &online);
    drawUserRowColumn(4, 0.3f, id, height2, &drawUserRowColumnActions, (const void*[2]) {&id, &conversationExists});

    nk_layout_row_end(this->context);
}

static void drawUsersList(void) {
    const float height = currentHeight(), decreasedHeight = (float) decreaseHeightIfNeeded((unsigned) height);

    char currentUserName[this->usernameSize + 1];
    SDL_memcpy(currentUserName, this->currentUserName, this->usernameSize);
    currentUserName[this->usernameSize] = 0;

    nk_layout_row_dynamic(this->context, decreasedHeight * 0.1f, 3);
    nk_label(this->context, WELCOME, NK_TEXT_ALIGN_CENTERED);
    nk_label(this->context, currentUserName, NK_TEXT_ALIGN_CENTERED);

    if (this->adminMode) {
        if (nk_button_label(this->context, SHUTDOWN_SERVER))
            (*(this->onServerShutdownRequested))();
    } else
        nk_spacer(this->context);

    nk_layout_row_dynamic(this->context, decreasedHeight * 0.1f, 5);
    nk_label(this->context, USERS_LIST, NK_TEXT_ALIGN_LEFT);
    for (unsigned i = 0; i < 3; i++) nk_spacer(this->context);
    if (nk_button_label(this->context, UPDATE)) (*(this->onUpdateUsersListClicked))();

    float heightMultiplier = 0.8f;
    if (this->loading) heightMultiplier -= 0.05f;
    if (this->currentSystemMessage) heightMultiplier -= 0.1f;

    nk_layout_row_dynamic(this->context, height * heightMultiplier, 1);
    char groupName[2] = {1, 0};
    if (!nk_group_begin(this->context, groupName, 0)) return;

    const unsigned size = listSize(this->usersList);
    for (unsigned i = 0; i < size; i++) {
        const User* user = (User*) listGet(this->usersList, i);

        char idString[MAX_U32_DEC_DIGITS_COUNT];
        assert(SDL_snprintf(idString, MAX_U32_DEC_DIGITS_COUNT, "%u", user->id) <= (int) MAX_U32_DEC_DIGITS_COUNT);

        drawUserRow(user->id, idString, user->name, user->conversationExists, user->online);
    }

    nk_group_end(this->context);
}

static void drawConversationMessage(
    float charHeight,
    const ConversationMessage* message,
    float timestampRatio,
    float fromRatio,
    float textRatio,
    struct nk_color fromUsernameColor,
    struct nk_color timestampColor
) {
    const float messageHeight = (
        (float) message->size / (
            ((float) this->width * 0.375f) /
            ((float) FONT_SIZE * 2 / 3)
        )
    ) * (float) FONT_SIZE;

    nk_layout_row_begin(this->context, NK_DYNAMIC, messageHeight < charHeight ? charHeight : messageHeight, 4);

    char text[this->maxMessageSize + 1];
    SDL_memcpy(text, message->text, this->maxMessageSize);
    text[this->maxMessageSize] = 0;

    nk_layout_row_push(this->context, timestampRatio);
    char* timestampText = (*(this->millisToDateTimeConverter))(message->timestamp);
    nk_label_colored(this->context, timestampText, NK_TEXT_ALIGN_CENTERED, timestampColor);
    SDL_free(timestampText);

    if (!message->from) {
        nk_layout_row_push(this->context, fromRatio);
        nk_label_colored(this->context, YOU, NK_TEXT_ALIGN_CENTERED, fromUsernameColor);

        nk_layout_row_push(this->context, textRatio);
        nk_spacer(this->context);

        nk_layout_row_push(this->context, textRatio);
        nk_text_wrap(this->context, text, (int) message->size);
    } else {
        nk_layout_row_push(this->context, fromRatio);
        nk_label_colored(this->context, message->from, NK_TEXT_ALIGN_CENTERED, fromUsernameColor);

        nk_layout_row_push(this->context, textRatio);
        nk_text_wrap(this->context, text, (int) message->size);

        nk_layout_row_push(this->context, textRatio);
        nk_spacer(this->context);
    }
}

static void onSendClicked(void) {
    (*(this->onSendClicked))(this->conversationMessage, this->enteredConversationMessageSize);
    SDL_memset(this->conversationMessage, 0, this->maxMessageSize);
    this->enteredConversationMessageSize = 0;
}

static void drawConversation(void) { // TODO: generate & sign messages from users on the client side
    const float height = currentHeight();

    char title[this->conversationNameSize + 1];
    SDL_memcpy(title, this->conversationName, this->conversationNameSize);

    nk_layout_row_begin(this->context, NK_DYNAMIC, height * 0.05f, 3);

    nk_layout_row_push(this->context, 0.15f);
    if (nk_button_label(this->context, BACK)) (*(this->onReturnFromConversationPageRequested))();

    nk_layout_row_push(this->context, 0.55f);
    nk_spacer(this->context);

    nk_layout_row_push(this->context, 0.3f);
    nk_label(this->context, title, NK_TEXT_ALIGN_RIGHT);

    nk_layout_row_end(this->context);

    float heightCorrector = 0.0f;
    if (this->loading) heightCorrector += 0.05f;
    if (this->currentSystemMessage) heightCorrector += 0.1f;

    nk_layout_row_dynamic(this->context, height * (0.85f - heightCorrector), 1);
    if (!nk_group_begin(this->context, title, 0)) return;

    struct nk_color
        timestampColor = {0, 128, 0, 255},
        fromUsernameColor = {0xff, 0xff, 0xff, 0x88};

    const bool aboveInitialWidth = this->width >= WINDOW_WIDTH * 2;
    const float timestampRatio = aboveInitialWidth ? 0.08f : 0.2f,
        fromRatio = aboveInitialWidth ? 0.05f : 0.1f,
        textRatio = aboveInitialWidth ? 0.435f : 0.35f;

    const unsigned size = listSize(this->conversationMessagesList);
    for (unsigned i = 0; i < size; i++) {
        const ConversationMessage* message = listGet(this->conversationMessagesList, i);

        drawConversationMessage(
            height * 0.05f,
            message,
            timestampRatio,
            fromRatio,
            textRatio,
            fromUsernameColor,
            timestampColor
        );
    }

    nk_group_end(this->context);

    nk_layout_row_begin(this->context, NK_DYNAMIC, (float) decreaseHeightIfNeeded((unsigned) height) * 0.1f, 2);

    nk_layout_row_push(this->context, 0.85f);
    nk_edit_string(
        this->context,
        NK_EDIT_BOX | NK_EDIT_MULTILINE | NK_EDIT_EDITOR,
        this->conversationMessage,
        (int*) &(this->enteredConversationMessageSize),
        (int) this->maxMessageSize,
        nk_filter_default
    );

    nk_layout_row_push(this->context, 0.15f);
    if (nk_button_label(this->context, SEND)) onSendClicked();

    nk_layout_row_end(this->context);
}

static void drawErrorIfNeeded(void) {
    this->systemMessageTicks++;

    if (!queueSize(this->systemMessagesQueue) && !this->currentSystemMessage) return; // TODO: optimize

    if (this->systemMessageTicks >= 60 * 3) { // 3 seconds
        this->systemMessageTicks = 0;

        if (this->currentSystemMessage) destroySystemMessage(this->currentSystemMessage);
        this->currentSystemMessage = NULL;

        if (queueSize(this->systemMessagesQueue)) this->currentSystemMessage = queuePop(this->systemMessagesQueue);
    }

    if (!this->currentSystemMessage) return;

    nk_layout_row_dynamic(this->context, 0, 1);

    if (this->currentSystemMessage->error) nk_label_colored(
        this->context,
        this->currentSystemMessage->text,
        NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_BOTTOM,
        (struct nk_color) { 0xff, 0, 0, 0xff }
    ); else nk_label(
        this->context,
        this->currentSystemMessage->text,
        NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_BOTTOM
    );
}

static void drawPage(void) {
    switch (this->state) {
        case STATE_INITIAL:
            drawSplashPage();
            break;
        case STATE_LOG_IN:
            drawLoginPage(true);
            break;
        case STATE_REGISTER:
            drawLoginPage(false);
            break;
        case STATE_USERS_LIST:
            drawUsersList();
            break;
        case STATE_CONVERSATION:
            drawConversation();
            break;
        default: assert(false);
    }

    if (this->loading) drawInfiniteProgressBar(0.05f);
    drawErrorIfNeeded();
}

void renderDraw(void) {
    SDL_SetRenderDrawColor(
        this->renderer,
        (int) this->colorf.r * 255,
        (int) this->colorf.g * 255,
        (int) this->colorf.b * 255,
        (int) this->colorf.a * 255
    );
    SDL_RenderClear(this->renderer);
    SDL_GetWindowSizeInPixels(this->window, (int*) &(this->width), (int*) &(this->height));

    if (nk_begin(
        this->context,
        TITLE,
        nk_rect(0, 0, (float) this->width, (float) this->height),
        NK_WINDOW_BORDER
    )) drawPage();

    nk_end(this->context);
    nk_sdl_render(NK_ANTI_ALIASING_ON);

    SDL_RenderPresent(this->renderer);
}

void renderClean(void) {
    if (!this) return;

    if (this->currentSystemMessage) destroySystemMessage(this->currentSystemMessage); // if window was closed before pause has been ended
    queueDestroy(this->systemMessagesQueue);

    SDL_free(this->currentUserName);
    SDL_free(this->conversationMessage);
    SDL_free(this->conversationName);

    SDL_DestroyMutex(this->uiQueriesMutex);
    SDL_free(this->enteredCredentialsBuffer);

    nk_sdl_shutdown();
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);

    SDL_free(this);
    this = NULL;
}
