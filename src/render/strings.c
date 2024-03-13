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
#include "strings.h"

const unsigned STRINGS = 53;
static StringsLanguages sLanguage = STRINGS_LANGUAGE_ENGLISH;

// English

static const char* const stringsEn[STRINGS] = {
    u8"Exchatge",
    u8"A secured text exchanger",
    u8"Log in",
    u8"Register",
    u8"Username",
    u8"Password",
    u8"Proceed",
    u8"Users list",
    u8"Start conversation",
    u8"Continue conversation",
    u8"Delete conversation",
    u8"Id",
    u8"Name",
    u8"Error",
    u8"Welcome ",
    u8"Shutdown the server",
    u8"Disconnected",
    u8"Unable to connect to the server",
    u8"Send",
    u8"Online",
    u8"Offline",
    u8"Back",
    u8"You",
    u8"Update",
    u8"Registration succeeded",
    u8"User is offline",
    u8"Invitation received",
    u8"You are invited to create conversation by user ",
    u8"Accept",
    u8"Decline",
    u8"Unable to decrypt the database",
    u8"Unable to create the conversation",
    u8"Conversation doesn't exist",
    u8"Conversation already exists",
    u8"File exchange requested",
    u8"File exchange requested by user",
    u8"with size of",
    u8"bytes",
    u8"Choose",
    u8"File",
    u8"File selection",
    u8"Cannot open the file",
    u8"Empty file path",
    u8"File is empty",
    u8"Unable to transmit file",
    u8"File is too big (> 20 mb)",
    u8"File transmitted",
    u8"Enter absolute path to the file",
    u8"Paste with Ctrl+V",
    u8"Auto logging in",
    u8"Admin actions",
    u8"Broadcast message",
    u8"All currently online users will receive this message, no encryption will be performed"
};

// Russian

static const char* const stringsRu[STRINGS] = {
    u8"Exchatge",
    u8"Защищенный текстовый обменник",
    u8"Войти",
    u8"Регистрация",
    u8"Имя пользователя",
    u8"Пароль",
    u8"Далее",
    u8"Пользователи",
    u8"Начать беседу",
    u8"Продолжить беседу",
    u8"Удалить беседу",
    u8"Номер",
    u8"Имя",
    u8"Ошибка",
    u8"Добро пожаловать ",
    u8"Отключить сервер",
    u8"Соединение закрыто",
    u8"Не удалось подключиться к серверу",
    u8"Отправить",
    u8"Онлайн",
    u8"Оффлайн",
    u8"Назад",
    u8"Вы",
    u8"Обновить",
    u8"Регистрация произведена успешно",
    u8"Пользователь не подключен",
    u8"Получено приглашение",
    u8"Вы приглашены создать беседу пользователем ",
    u8"Принять",
    u8"Отвергнуть",
    u8"Не удалось расшифровать базу данных",
    u8"Не удалось создать беседу",
    u8"Беседа не существует",
    u8"Беседа уже существует",
    u8"Запрошен обмен файлом",
    u8"Запрошен обмен файлом пользователем",
    u8"с размером",
    u8"байтов",
    u8"Выбрать",
    u8"Файл",
    u8"Выбор файла",
    u8"Не удалось открыть файл",
    u8"Пустой путь до файла",
    u8"Файл пуст",
    u8"Не удалось передать файл",
    u8"Файл слишком большой (> 20 мб)",
    u8"Файл передан",
    u8"Введите абсолютный путь до файла",
    u8"Вставить с помощью Ctrl+V",
    u8"Входить автоматически",
    u8"Администрировать",
    u8"Рассылка",
    u8"Все пользователи, которые сейчас подключены, получат это сообщение, дополнительное шифрование произведено не будет"
};

// End

void stringsSetLanguage(StringsLanguages language) { sLanguage = language; }

const char* stringsString(unsigned id) {
    assert(id < STRINGS);
    switch (sLanguage) {
        case STRINGS_LANGUAGE_ENGLISH: return stringsEn[id];
        case STRINGS_LANGUAGE_RUSSIAN: return stringsRu[id];
        default: assert(0);
    }
}

const unsigned TITLE = 0;
const unsigned SUBTITLE = 1;
const unsigned LOG_IN = 2;
const unsigned REGISTER = 3;
const unsigned USERNAME = 4;
const unsigned PASSWORD = 5;
const unsigned PROCEED = 6;
const unsigned USERS_LIST = 7;
const unsigned START_CONVERSATION = 8;
const unsigned CONTINUE_CONVERSATION = 9;
const unsigned DELETE_CONVERSATION = 10;
const unsigned ID_TEXT = 11;
const unsigned NAME_TEXT = 12;
const unsigned ERROR_TEXT = 13;
const unsigned WELCOME = 14;
const unsigned SHUTDOWN_SERVER = 15;
const unsigned DISCONNECTED = 16;
const unsigned UNABLE_TO_CONNECT_TO_THE_SERVER = 17;
const unsigned SEND = 18;
const unsigned ONLINE = 19;
const unsigned OFFLINE = 20;
const unsigned BACK = 21;
const unsigned YOU = 22;
const unsigned UPDATE = 23;
const unsigned REGISTRATION_SUCCEEDED = 24;
const unsigned USER_IS_OFFLINE = 25;
const unsigned INVITATION_RECEIVED = 26;
const unsigned YOU_ARE_INVITED_TO_CREATE_CONVERSATION_BY_USER = 27;
const unsigned ACCEPT = 28;
const unsigned DECLINE = 29;
const unsigned UNABLE_TO_DECRYPT_DATABASE = 30;
const unsigned UNABLE_TO_CREATE_CONVERSATION = 31;
const unsigned CONVERSATION_DOESNT_EXIST = 32;
const unsigned CONVERSATION_ALREADY_EXISTS = 33;
const unsigned FILE_EXCHANGE_REQUESTED = 34;
const unsigned FILE_EXCHANGE_REQUESTED_BY_USER = 35;
const unsigned WITH_SIZE_OF = 36;
const unsigned BYTES = 37;
const unsigned CHOOSE = 38;
const unsigned FILE_TEXT = 39;
const unsigned FILE_SELECTION = 40;
const unsigned CANNOT_OPEN_FILE = 41;
const unsigned EMPTY_FILE_PATH = 42;
const unsigned FILE_IS_EMPTY = 43;
const unsigned UNABLE_TO_TRANSMIT_FILE = 44;
const unsigned FILE_IS_TOO_BIG = 45;
const unsigned FILE_TRANSMITTED = 46;
const unsigned ENTER_ABSOLUTE_PATH_TO_FILE = 47;
const unsigned PASTE_WITH_CTRL_V = 48;
const unsigned AUTO_LOGGING_IN = 49;
const unsigned ADMIN_ACTIONS = 50;
const unsigned BROADCAST_MESSAGE = 51;
const unsigned BROADCAST_HINT = 52;
