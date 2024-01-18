/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023-2024  Vadim Nikolaev (https://github.com/vadniks)
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

static const unsigned UI_UPDATE_PERIOD = 1000 / 60; // 16.7, at most 60 frames per second
static const unsigned NET_UPDATE_PERIOD = 1000 / 2; // 500, at most 2 updates per second

typedef struct {
    LifecycleAsyncActionFunction function;
    void* nullable parameter;
    unsigned long delayMillis; // delay is happened when the queue pushes this action, so if there are many waiting actions in the queue, the delay may be longer then expected, delay can be zero
} AsyncAction;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    atomic bool running;
    Queue* asyncActionsQueue; // <AsyncAction*>
    SDL_Thread* asyncActionsThread;
    SDL_TimerID netUpdateThreadId;
)
#pragma clang diagnostic pop

void lifecycleAsync(LifecycleAsyncActionFunction function, void* nullable parameter, unsigned long delayMillis) {
    assert(this);
    if (!(this->running)) return;

    AsyncAction* action = SDL_malloc(sizeof *action);
    action->function = function;
    action->parameter = parameter;
    action->delayMillis = delayMillis;

    queuePush(this->asyncActionsQueue, action);
}

static void asyncActionsThreadLooper(void) {
    while (this->running) {
        if (!queueSize(this->asyncActionsQueue)) {
            lifecycleSleep(100);
            continue;
        }

        AsyncAction* action = queuePop(this->asyncActionsQueue);
        if (action->delayMillis > 0) lifecycleSleep(action->delayMillis);
        (*(action->function))(action->parameter);
        SDL_free(action);
    }
}

static unsigned netUpdateLopper(void) { // TODO: add groups and broadcast
    if (this->running) {
        logicNetListen(); // gets called in another thread (not even in asyncActionsThread) so the incoming messages can be processed while sending/receiving files and/or setting up conversation
        return NET_UPDATE_PERIOD;
    } else
        return 0;
}

bool lifecycleInit(void) {
    this = SDL_malloc(sizeof *this);
    this->running = true;
    this->asyncActionsQueue = queueInit((QueueDeallocator) &SDL_free);
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
        &logicOnUpdateUsersListClicked,
        LOGIC_MAX_FILE_PATH_SIZE,
        &logicOnFileChooserRequested,
        &logicFileChooseResultHandler,
        &logicOnAutoLoggingInChanged,
        &logicAutoLoggingInSupplier,
        &logicOnAdminActionsPageRequested,
        &logicOnBroadcastMessageSendRequested
    );
    logicInit();
    renderSetMaxMessageSizeAndInitConversationMessageBuffer(logicMaxMessagePlainPayloadSize()); // TODO: unite all setters to one global setter
    renderSetAdminMode(logicIsAdminMode());
    renderSetTheme((RenderThemes) logicIsDarkTheme());
    renderSetUsersList(logicUsersList());
    renderSetMessagesList(logicMessagesList());

    this->netUpdateThreadId = SDL_AddTimer(NET_UPDATE_PERIOD, (SDL_TimerCallback) &netUpdateLopper, NULL);

    return true;
}

void lifecycleSleep(unsigned long delayMillis) {
    assert(delayMillis > 0 && delayMillis <= 10000);
    const ldiv_t dv = ldiv((long) delayMillis, (long) 1e3f);
    nanosleep(&((struct timespec) {dv.quot, dv.rem * (long) 1e6f}), NULL);
}

static bool processEvents(void) {
    SDL_Event event;
    renderInputBegan();

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;
        logicProcessEvent(&event);
        renderProcessEvent(&event);
    }

    renderInputEnded();
    return false;
}

void lifecycleLoop(void) {
    unsigned long startMillis, differenceMillis;
    while (this->running) {
        startMillis = logicCurrentTimeMillis();

        if (processEvents()) {
            this->running = false;
            lifecycleClean();
            break;
        }

        renderDraw();

        differenceMillis = logicCurrentTimeMillis() - startMillis;
        if ((unsigned long) UI_UPDATE_PERIOD > differenceMillis)
            lifecycleSleep((unsigned long) UI_UPDATE_PERIOD - differenceMillis);

//        SDL_Log("fps: %u", (unsigned) (1000.0f / (float) (logicCurrentTimeMillis() - startMillis)));
    }
}

void lifecycleClean(void) {
    if (!this) return;

    SDL_RemoveTimer(this->netUpdateThreadId);
    SDL_WaitThread(this->asyncActionsThread, NULL);
    queueDestroy(this->asyncActionsQueue);

    logicClean();
    renderClean();

    SDL_free(this);
    this = NULL;

    SDL_Quit();
}
