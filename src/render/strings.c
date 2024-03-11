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
