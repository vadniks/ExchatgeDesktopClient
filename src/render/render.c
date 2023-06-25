
#include <assert.h>
//#include "nuklearDefs.h" - cmake automatically includes precompiled version of this header
#include "../defs.h"
#include "render.h"

#define SYNCHRONIZED_BEGIN assert(!SDL_LockMutex(this->uiQueriesMutex)); {
#define SYNCHRONIZED_END assert(!SDL_UnlockMutex(this->uiQueriesMutex)); }
#define SYNCHRONIZED(x) SYNCHRONIZED_BEGIN x SYNCHRONIZED_END

STATIC_CONST_UNSIGNED WINDOW_WIDTH = 16 * 50;
STATIC_CONST_UNSIGNED WINDOW_HEIGHT = 9 * 50;
STATIC_CONST_UNSIGNED WINDOW_MINIMAL_WIDTH = WINDOW_WIDTH / 2;
STATIC_CONST_UNSIGNED WINDOW_MINIMAL_HEIGHT = WINDOW_HEIGHT / 2;
STATIC_CONST_UNSIGNED MAX_U32_DEC_DIGITS_COUNT = 10; // 0xffffffff = 4294967295 10 digits

STATIC_CONST_UNSIGNED STATE_INITIAL = 0;
STATIC_CONST_UNSIGNED STATE_LOG_IN = 1;
STATIC_CONST_UNSIGNED STATE_REGISTER = 2;
STATIC_CONST_UNSIGNED STATE_USERS_LIST = 3;
STATIC_CONST_UNSIGNED STATE_CONVERSATION = 4;

STATIC_CONST_STRING TITLE = "Exchatge";
STATIC_CONST_STRING SUBTITLE = "A secured text exchanger";
STATIC_CONST_STRING LOG_IN = "Log in";
STATIC_CONST_STRING REGISTER = "Register";
STATIC_CONST_STRING USERNAME = "Username";
STATIC_CONST_STRING PASSWORD = "Password";
STATIC_CONST_STRING PROCEED = "Proceed";
STATIC_CONST_STRING CONNECTED_USERS = "Connected users";
STATIC_CONST_STRING START_CONVERSATION = "Start conversation";
STATIC_CONST_STRING CONTINUE_CONVERSATION = "Continue conversation";
STATIC_CONST_STRING DELETE_CONVERSATION = "Delete conversation";
STATIC_CONST_STRING ID_TEXT = "Id";
STATIC_CONST_STRING NAME_TEXT = "Name";
STATIC_CONST_STRING EMPTY_TEXT = "";
STATIC_CONST_STRING ERROR_TEXT = "Error";
STATIC_CONST_STRING WELCOME_ADMIN = "Welcome admin!";
STATIC_CONST_STRING SHUTDOWN_SERVER = "Shutdown the server";

const unsigned RENDER_MAX_MESSAGE_TEXT_SIZE = 64;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    unsigned width;
    unsigned height;
    SDL_Window* window;
    SDL_Renderer* renderer;
    struct nk_context* context;
    struct nk_colorf colorf;
    unsigned state;
    unsigned usernameSize;
    unsigned passwordSize;
    RenderCredentialsReceivedCallback onCredentialsReceived;
    RenderCredentialsRandomFiller credentialsRandomFiller;
    unsigned enteredUsernameSize;
    unsigned enteredPasswordSize;
    char* enteredCredentialsBuffer;
    RenderLogInRegisterPageQueriedByUserCallback onLoginRegisterPageQueriedByUser;
    bool showSystemMessage;
    bool isErrorMessageSystem;
    char systemMessageText[RENDER_MAX_MESSAGE_TEXT_SIZE]; // message from the system (application)
    SDL_mutex* uiQueriesMutex;
    const List* usersList; // allocated elsewhere
    RenderUserForConversationChosenCallback onUserForConversationChosen;
    const List* messagesList; // allocated elsewhere, conversation messages
    unsigned maxMessageSize;
    char* conversationName; // conversation name or the name of the recipient
    unsigned conversationNameSize;
    bool adminMode;
    RenderOnServerShutdownRequested onServerShutdownRequested;
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

void renderInit(
    unsigned usernameSize,
    unsigned passwordSize,
    RenderCredentialsReceivedCallback onCredentialsReceived,
    RenderCredentialsRandomFiller credentialsRandomFiller,
    RenderLogInRegisterPageQueriedByUserCallback onLoginRegisterPageQueriedByUser,
    RenderUserForConversationChosenCallback onUserForConversationChosen,
    unsigned maxMessageSize,
    unsigned conversationNameSize,
    RenderOnServerShutdownRequested onServerShutdownRequested
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
    this->showSystemMessage = false;
    this->isErrorMessageSystem = false;
    SDL_memset(this->systemMessageText, 0, RENDER_MAX_MESSAGE_TEXT_SIZE);

    this->uiQueriesMutex = SDL_CreateMutex();
    assert(this->uiQueriesMutex);

    this->usersList = NULL;
    this->onUserForConversationChosen = onUserForConversationChosen;
    this->messagesList = NULL;
    this->maxMessageSize = maxMessageSize;
    this->conversationName = SDL_calloc(this->usernameSize, sizeof(char));

    this->conversationNameSize = conversationNameSize;
    assert(conversationNameSize > 0);

    this->adminMode = false;
    this->onServerShutdownRequested = onServerShutdownRequested;

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

    SDL_SetWindowMinimumSize(this->window, (int) WINDOW_MINIMAL_WIDTH, (int) WINDOW_MINIMAL_HEIGHT);

    int renderW, renderH, windowW, windowH;
    float scaleX, scaleY, fontScale;

    SDL_GetRendererOutputSize(this->renderer, &renderW, &renderH);
    SDL_GetWindowSize(this->window, &windowW, &windowH);

    scaleX = (float) renderW / (float) windowW;
    scaleY = (float) renderH / (float) windowH;

    SDL_RenderSetScale(this->renderer, scaleX, scaleY);
    fontScale = scaleY;

    this->context = nk_sdl_init(this->window, this->renderer);
    struct nk_font_config config = nk_font_config(10);

    struct nk_font_atlas* atlas = NULL;
    nk_sdl_font_stash_begin(&atlas);

    struct nk_font* font = nk_font_atlas_add_default(atlas, 14 * fontScale, &config);
    nk_sdl_font_stash_end();

    font->handle.height /= fontScale;
    nk_style_set_font(this->context, &(font->handle));

    setStyle();

    this->colorf = (struct nk_colorf) { 0.10f, 0.18f, 0.24f, 1.00f };
}

void renderSetAdminMode(bool mode) {
    assert(this);
    this->adminMode = mode;
}

List* renderInitUsersList(void) { return listInit((ListDeallocator) renderDestroyUser); }

RenderUser* renderCreateUser(unsigned id, const char* name, bool conversationExists) {
    assert(this);
    bool foundNullTerminator = false;
    for (unsigned i = 0; i < this->usernameSize; i++) // TODO: extract this piece of null-terminator finder code into separate function/macro
        if (name[i] == '\0') {
            assert(i <= this->usernameSize);
            foundNullTerminator = true;
            break;
        }
    assert(foundNullTerminator);

    RenderUser* user = SDL_malloc(sizeof *user);
    user->id = id;
    user->name = SDL_calloc(this->usernameSize, sizeof(char));
    user->conversationExists = conversationExists;
    SDL_memcpy(user->name, name, this->usernameSize);
    return user;
}

void renderDestroyUser(RenderUser* user) {
    SDL_free(user->name);
    SDL_free(user);
}

void renderSetUsersList(const List* usersList) {
    assert(this);
    this->usersList = usersList;
}

List* renderInitMessagesList(void) { return listInit((ListDeallocator) &renderDestroyMessage); }

RenderMessage* renderCreateMessage(unsigned long timestamp, bool fromThisClient, const char* text, unsigned size) {
    assert(this && size > 0 && size - 1 <= this->maxMessageSize);

    RenderMessage* message = SDL_malloc(sizeof *message);
    message->timestamp = timestamp;
    message->fromThisClient = fromThisClient;

    message->text = SDL_calloc(size, this->maxMessageSize); // sets all bytes to zero so the string consists only of null-terminators so no need to insert one
    SDL_memcpy(message->text, text, size);

    message->size = size;

    return message;
}

void renderDestroyMessage(RenderMessage* message) {
    SDL_free(message->text);
    SDL_free(message);
}

void renderSetMessagesList(const List* messagesList) {
    assert(this);
    this->messagesList = messagesList;
}

void renderInputBegan(void) {
    assert(this);
    nk_input_begin(this->context);
}

void renderProcessEvent(SDL_Event* event) {
    assert(this);
    nk_sdl_handle_event(event);
}

void renderInputEnded(void) {
    assert(this);
    nk_input_end(this->context);
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

void renderShowUsersList(void) {
    assert(this);
    SYNCHRONIZED(this->state = STATE_USERS_LIST;)
}

void renderShowConversation(const char* conversationName) {
    assert(this);
    SYNCHRONIZED_BEGIN

    SDL_memcpy(this->conversationName, conversationName, this->conversationNameSize);
    this->state = STATE_CONVERSATION;

    SYNCHRONIZED_END
}

void renderShowSystemMessage(const char* message, bool error) {
    assert(this);

    SYNCHRONIZED_BEGIN
    SDL_memset(this->systemMessageText, 0, RENDER_MAX_MESSAGE_TEXT_SIZE);

    bool foundNullTerminator = false;
    for (unsigned i = 0; i < RENDER_MAX_MESSAGE_TEXT_SIZE; i++) {
        this->systemMessageText[i] = message[i];

        if (message[i] == '\0') {
            assert(i > 0);
            foundNullTerminator = true;
            break;
        }
    }
    assert(foundNullTerminator);

    this->isErrorMessageSystem = error;
    this->showSystemMessage = true;
    SYNCHRONIZED_END
}

void renderHideSystemMessage(void) {
    assert(this);
    SYNCHRONIZED_BEGIN

    this->showSystemMessage = false;
    this->isErrorMessageSystem = false;

    SYNCHRONIZED_END
}

void renderShowSystemError(void) {
    assert(this);
    SYNCHRONIZED_BEGIN

    SDL_memcpy(this->systemMessageText, ERROR_TEXT, 6); // Error + \0 = 6 chars
    this->isErrorMessageSystem = true;
    this->showSystemMessage = true;

    SYNCHRONIZED_END
}

static void drawSplashPage(void) {
    static char anim = '|'; // value is saved between function calls
    const unsigned maxCounterValue = 5;
    static unsigned counter = maxCounterValue;

    if (counter) {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue" // it's value used in the next time this function is called 'cause it's a static variable
        counter = counter < maxCounterValue ? counter + 1 : 0;
#pragma clang diagnostic pop
    } else {
        switch (anim) { // anim: | / - \...
            case '|':
                anim = '/';
                break;
            case '/':
                anim = '-';
                break;
            case '-':
                anim = '\\';
                break;
            case '\\':
                anim = '|';
                break;
            default: assert(false);
        }
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
        counter++;
#pragma clang diagnostic pop
    }

    nk_layout_row_dynamic(this->context, 0, 1);
    nk_label(this->context, TITLE, NK_TEXT_CENTERED);
    nk_label(this->context, SUBTITLE, NK_TEXT_CENTERED);

    nk_layout_row_dynamic(this->context, 0, 1);
    char animText[2] = {anim, '\0'};
    nk_label(this->context, animText, NK_TEXT_ALIGN_CENTERED);
}

static void onProceedClickedAfterLogInRegister(bool logIn) {
    (*(this->onCredentialsReceived))(
        this->enteredCredentialsBuffer,
        this->enteredCredentialsBuffer + this->usernameSize,
        logIn
    );

    (*(this->credentialsRandomFiller))(this->enteredCredentialsBuffer, this->usernameSize + this->passwordSize);
    this->enteredUsernameSize = 0;
    this->enteredPasswordSize = 0;
}

static void drawLoginPage(bool logIn) {
    nk_layout_row_dynamic(this->context, 0, 1);
    nk_label(this->context, logIn ? LOG_IN : REGISTER, NK_TEXT_CENTERED);

    nk_layout_row_dynamic(this->context, 0, 2);
    nk_label(this->context, USERNAME, NK_TEXT_ALIGN_LEFT);

    nk_edit_string(
        this->context,
        NK_EDIT_FIELD,
        this->enteredCredentialsBuffer,
        (int*) &(this->enteredUsernameSize),
        (int) this->usernameSize,
        nk_filter_default
    );

    nk_label(this->context, PASSWORD, NK_TEXT_ALIGN_LEFT);

    nk_edit_string(
        this->context,
        NK_EDIT_FIELD,
        this->enteredCredentialsBuffer + this->usernameSize,
        (int*) &(this->enteredPasswordSize),
        (int) this->passwordSize,
        nk_filter_default
    );

    nk_layout_row_dynamic(this->context, 0, 2);
    if (nk_button_label(this->context, PROCEED)) onProceedClickedAfterLogInRegister(logIn);
    if (nk_button_label(this->context, logIn ? REGISTER : LOG_IN)) (*(this->onLoginRegisterPageQueriedByUser))(!logIn);
}

static void drawDivider(const char* groupName) {
    nk_layout_row_dynamic(this->context, 5, 1);
    if (nk_group_begin(this->context, groupName, 0)) {
        nk_spacer(this->context);
        nk_group_end(this->context);
    }
}

static void drawUserRow(unsigned id, const char* nullable idString, const char* nullable name, int mode) { // mode: -1 - stub, 0 - conversation doesn't exist, 1 - exists
    nk_layout_row_dynamic(this->context, 0, 3);
    nk_label(this->context, idString ? idString : ID_TEXT, NK_TEXT_ALIGN_LEFT);
    nk_label(this->context, name ? name : NAME_TEXT, NK_TEXT_ALIGN_CENTERED);

    if (mode == -1) {
        nk_spacer(this->context);
        drawDivider(EMPTY_TEXT);
        return;
    }

    if (mode) {
        if (nk_button_label(this->context, CONTINUE_CONVERSATION))
            (*(this->onUserForConversationChosen))(id, RENDER_CONTINUE_CONVERSATION);
    } else {
        if (nk_button_label(this->context, START_CONVERSATION))
            (*(this->onUserForConversationChosen))(id, RENDER_START_CONVERSATION);
    }

    if (mode) {
        nk_layout_row_dynamic(this->context, 0, 3);
        nk_spacer(this->context);
        nk_spacer(this->context);

        if (nk_button_label(this->context, DELETE_CONVERSATION))
            (*(this->onUserForConversationChosen))(id, RENDER_DELETE_CONVERSATION);
    }
}

static void drawUsersList(void) {
    if (this->adminMode) {
        nk_layout_row_dynamic(this->context, 0, 2);
        nk_label(this->context, WELCOME_ADMIN, NK_TEXT_ALIGN_LEFT);

        if (nk_button_label(this->context, SHUTDOWN_SERVER))
            (*(this->onServerShutdownRequested))();

        nk_spacer(this->context);
    }

    nk_layout_row_dynamic(this->context, 0, 1);
    nk_label(this->context, CONNECTED_USERS, NK_TEXT_ALIGN_CENTERED);
    nk_spacer(this->context);

    drawUserRow(0xffffffff, NULL, NULL, -1);

    const unsigned size = listSize(this->usersList);
    for (unsigned i = 0; i < size; i++) {
        RenderUser* user = (RenderUser*) listGet(this->usersList, i);

        char idString[MAX_U32_DEC_DIGITS_COUNT];
        assert(SDL_snprintf(idString, MAX_U32_DEC_DIGITS_COUNT, "%u", user->id) <= (int) MAX_U32_DEC_DIGITS_COUNT);

        drawUserRow(user->id, idString, user->name, user->conversationExists);

        if (i < size - 1) {
            char groupName[MAX_U32_DEC_DIGITS_COUNT];
            assert(SDL_snprintf(groupName, MAX_U32_DEC_DIGITS_COUNT, "%u", i) <= (int) MAX_U32_DEC_DIGITS_COUNT);
            drawDivider(groupName);
        }
    }
}

static void drawConversation(void) {
    char title[this->conversationNameSize + 1];
    SDL_memcpy(title, this->conversationName, this->conversationNameSize);

    nk_layout_row_dynamic(this->context, 0, 1);
    nk_label(this->context, title, NK_TEXT_ALIGN_CENTERED);
    nk_spacer(this->context);

    nk_layout_row_dynamic(this->context, 0, 2);

    const unsigned size = listSize(this->messagesList);
    for (unsigned i = 0; i < size; i++) {
        RenderMessage* message = listGet(this->messagesList, i);

        char text[this->maxMessageSize + 1];
        SDL_memcpy(text, message->text, this->maxMessageSize);
        text[this->maxMessageSize] = '\0';

        if (message->fromThisClient) {
            nk_spacer(this->context);
            nk_label(this->context, text, NK_TEXT_ALIGN_RIGHT);
        } else {
            nk_label(this->context, text, NK_TEXT_ALIGN_LEFT);
            nk_spacer(this->context);
        }
    }
}

static void drawError(void) {
    nk_spacer(this->context);
    nk_layout_row_dynamic(this->context, 0, 1);

    if (this->isErrorMessageSystem) nk_label_colored(
        this->context,
        this->systemMessageText,
        NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_BOTTOM,
        (struct nk_color) { 0xff, 0, 0, 0xff }
    ); else nk_label(
        this->context,
        this->systemMessageText,
        NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_BOTTOM
    );
}

static void drawPage(void) {
    nk_spacer(this->context);

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

    if (this->showSystemMessage) drawError();
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

    SDL_free(this->conversationName);

    SDL_DestroyMutex(this->uiQueriesMutex);
    SDL_free(this->enteredCredentialsBuffer);

    nk_sdl_shutdown();
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);

    SDL_free(this);
    this = NULL;
}
