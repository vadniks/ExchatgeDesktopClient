
#include <assert.h>
#include <unistd.h>
#include "render/render.h"
#include "defs.h"
#include "net.h"
#include "logic.h"
#include "lifecycle.h"

static const unsigned UI_UPDATE_PERIOD = 1000 / 60;
static const unsigned NET_UPDATE_PERIOD = 60 / 15;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    volatile bool running;
    SDL_TimerID threadsSynchronizerTimerId;
    unsigned updateThreadCounter;
    SDL_cond* netUpdateCond;
    SDL_mutex* netUpdateLock;
    SDL_Thread* netThread;
)
#pragma clang diagnostic pop

static void updateSynchronized(Function action, SDL_cond* cond, SDL_mutex* lock) {
    SDL_LockMutex(lock);
    action(NULL);
    SDL_CondWait(cond, lock);
    SDL_UnlockMutex(lock);
}

static void netThread(void)
{ while (this->running) updateSynchronized((Function) &logicNetListen, this->netUpdateCond, this->netUpdateLock); }

static unsigned synchronizeThreadUpdates(void) {
    if (this->updateThreadCounter == NET_UPDATE_PERIOD) {
        this->updateThreadCounter = 1;
        SDL_CondSignal(this->netUpdateCond);
    } else
        this->updateThreadCounter++;

    return this->running ? UI_UPDATE_PERIOD : 0;
}

static void async(void (*action)(void)) {
    SDL_DetachThread(SDL_CreateThread(
        (int (*)(void*)) action, // TODO: thread data race possible when app has been exited but the thread still executes - might cause dereference error on accessing 'this
        "asyncActionThread",
        NULL
    ));
}

static void delayed(unsigned seconds, void (*action)(void)) {
    sleep(seconds);
    (*(action))(); // TODO: thread data race possible when app has been exited but the thread still executes - might cause dereference error on accessing 'this
}

static void showLogInUiDelayed(void) { delayed(1, &renderShowLogIn); } // causes render module to show splash page until logIn page is queried one second later

bool lifecycleInit(unsigned argc, const char** argv) { // TODO: expose net module's flags in it's header
    this = SDL_malloc(sizeof *this);
    this->running = true;
    this->updateThreadCounter = 1;
    this->netUpdateCond = SDL_CreateCond();
    this->netUpdateLock = SDL_CreateMutex();
    this->netThread = SDL_CreateThread((int (*)(void*)) &netThread, "netThread", NULL);

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    assert(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER));

    renderInit(
        NET_USERNAME_SIZE,
        NET_UNHASHED_PASSWORD_SIZE,
        &logicOnCredentialsReceived,
        &logicCredentialsRandomFiller,
        &logicOnLoginRegisterPageQueriedByUser,
        &logicOnUserForConversationChosen,
        NET_MESSAGE_BODY_SIZE,
        NET_USERNAME_SIZE,
        &logicOnServerShutdownRequested,
        &logicOnReturnFromConversationPageRequested,
        &logicMillisToDateTime,
        &logicOnSendClicked
    );
    logicInit(argc, argv, &async, &delayed);
    renderSetAdminMode(logicIsAdminMode());
    renderSetUsersList(logicUsersList());
    renderSetMessagesList(logicMessagesList());
    async(&showLogInUiDelayed);

    this->threadsSynchronizerTimerId = SDL_AddTimer(
        UI_UPDATE_PERIOD,
        (unsigned (*)(unsigned, void*)) &synchronizeThreadUpdates,
        NULL
    );

    return true;
}

static bool processEvents(void) {
    SDL_Event event;
    renderInputBegan();

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;
        renderProcessEvent(&event);
    }

    renderInputEnded();
    return false;
}

static void stopApp(void) {
    this->running = false;
    SDL_CondSignal(this->netUpdateCond);
}

void lifecycleLoop(void) {
    while (this->running) {

        if (processEvents()) {
            stopApp();
            lifecycleClean();
            break;
        }

        renderDraw();
    }
}

void lifecycleClean(void) {
    if (!this) return;

    logicClean();

    SDL_RemoveTimer(this->threadsSynchronizerTimerId);

    renderClean();

    SDL_WaitThread(this->netThread, NULL);
    SDL_DestroyMutex(this->netUpdateLock);
    SDL_DestroyCond(this->netUpdateCond);

    SDL_free(this);
    this = NULL;

    SDL_Quit();
}
