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
#include <time.h>
#include <stdlib.h>
#include "render/render.h"
#include "defs.h"
#include "net.h"
#include "logic.h"
#include "collections/queue.h"
#include "lifecycle.h"

static const unsigned UI_UPDATE_PERIOD = 1000 / 60;
static const unsigned NET_UPDATE_PERIOD = 60 / 15;

typedef struct {
    LifecycleAsyncActionFunction function;
    void* nullable parameter;
    unsigned long delayMillis; // delay is happened when the queue pushes this action, so if there are many waiting actions in the queue, the delay may be longer then expected, delay can be zero
} AsyncAction;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    atomic bool running;
    SDL_TimerID threadsSynchronizerTimerId;
    unsigned updateThreadCounter;
    SDL_cond* netUpdateCond;
    SDL_mutex* netUpdateLock;
    SDL_Thread* netThread;
    Queue* asyncActionsQueue; // <AsyncAction*>
    SDL_Thread* asyncActionsThread;
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

void lifecycleAsync(LifecycleAsyncActionFunction function, void* nullable parameter, unsigned long delayMillis) {
    assert(this);
    if (!(this->running)) return;

    AsyncAction* action = SDL_malloc(sizeof *action);
    action->function = function;
    action->parameter = parameter;
    action->delayMillis = delayMillis;

    queuePush(this->asyncActionsQueue, action);
}

static void sleep(unsigned long delayMillis) {
    assert(delayMillis > 0 && delayMillis <= 10000);
    const ldiv_t dv = ldiv((long) delayMillis, (long) 1e3f);

    struct timespec timespec;
    timespec.tv_sec = dv.quot;
    timespec.tv_nsec = dv.rem * (long) 1e6f;

    nanosleep(&timespec, NULL);
}

static void asyncActionDeallocator(AsyncAction* action) { SDL_free(action); }

static void asyncActionsThreadLooper(void) {
    while (this->running) {
        if (!queueSize(this->asyncActionsQueue)) continue;
        AsyncAction* action = queuePop(this->asyncActionsQueue);

        if (action->delayMillis > 0) sleep(action->delayMillis);
        (*(action->function))(action->parameter);

        asyncActionDeallocator(action);
    }
}

bool lifecycleInit(unsigned argc, const char** argv) {
    this = SDL_malloc(sizeof *this);
    this->running = true;
    this->updateThreadCounter = 1;
    this->netUpdateCond = SDL_CreateCond();
    this->netUpdateLock = SDL_CreateMutex();
    this->netThread = SDL_CreateThread((int (*)(void*)) &netThread, "netThread", NULL);
    this->asyncActionsQueue = queueInit((QueueDeallocator) &asyncActionDeallocator);
    this->asyncActionsThread = SDL_CreateThread((SDL_ThreadFunction) &asyncActionsThreadLooper, "asyncActionsThread", NULL);

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1"); // TODO: optimize ui for highDpi displays
    assert(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER));

    renderInit(
        NET_USERNAME_SIZE,
        NET_UNHASHED_PASSWORD_SIZE,
        &logicOnCredentialsReceived,
        &logicCredentialsRandomFiller,
        &logicOnLoginRegisterPageQueriedByUser,
        &logicOnUserForConversationChosen,
        NET_USERNAME_SIZE,
        &logicOnServerShutdownRequested,
        &logicOnReturnFromConversationPageRequested,
        &logicMillisToDateTime,
        &logicOnSendClicked,
        &logicOnUpdateUsersListClicked
    );
    logicInit(argc, argv);
    renderSetMaxMessageSize(logicUnencryptedMessageBodySize());
    renderSetAdminMode(logicIsAdminMode());
    renderSetUsersList(logicUsersList());
    renderSetMessagesList(logicMessagesList());

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
    unsigned long startMillis, differenceMillis;
    while (this->running) {

        if (processEvents()) {
            stopApp();
            lifecycleClean();
            break;
        }

        startMillis = logicCurrentTimeMillis();
        renderDraw();

        do differenceMillis = logicCurrentTimeMillis() - startMillis;
        while (differenceMillis < UI_UPDATE_PERIOD);
    }
}

void lifecycleClean(void) {
    if (!this) return;

    SDL_WaitThread(this->asyncActionsThread, NULL);
    queueDestroy(this->asyncActionsQueue);

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
