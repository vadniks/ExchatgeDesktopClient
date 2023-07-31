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

#include <SDL_stdinc.h>
#include <assert.h>
#include "conversationMessage.h"

List* conversationMessageInitList(void) { return listInit((ListDeallocator) &conversationMessageDestroy); }

ConversationMessage* conversationMessageCreate( // from: name of the sender (is copied) (length is equal to fromSize which is in range (0, NET_USERNAME_SIZE]), null if from this client; text: text (whose length = textSize == size and 0 < length <= NET_MESSAGE_BODY_SIZE) is copied
    unsigned long timestamp,
    const char* nullable from,
    unsigned fromSize,
    const char* text,
    unsigned textSize
) {
    assert((from && fromSize > 0 || !from && !fromSize) && textSize > 0);

    ConversationMessage* message = SDL_malloc(sizeof *message);
    message->timestamp = timestamp;

    if (from) {
        message->from = SDL_calloc(fromSize, sizeof(char));
        SDL_memcpy(message->from, from, fromSize);
    } else
        message->from = NULL;

    message->text = SDL_calloc(textSize, sizeof(char));
    SDL_memcpy(message->text, text, textSize); // null-terminator should be inserted in buffer in which the text is copied

    message->size = textSize;

    return message;
}

void conversationMessageDestroy(ConversationMessage* message) {
    SDL_free(message->from);
    SDL_free(message->text);
    SDL_free(message);
}
