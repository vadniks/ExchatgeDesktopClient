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
#include <unistd.h>
#include "crypto.h"
#include "options.h"

STATIC_CONST_STRING OPTIONS_FILE = "options.txt";
STATIC_CONST_UNSIGNED MAX_FILE_SIZE = 1 << 10;

STATIC_CONST_STRING ADMIN_OPTION = "admin";
STATIC_CONST_STRING HOST_OPTION = "host";
STATIC_CONST_STRING PORT_OPTION = "port";
STATIC_CONST_STRING SSPK_OPTION = "sspk";
STATIC_CONST_STRING CREDENTIALS_OPTION = "credentials";

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // they're all used despite what the SAT says
THIS(
    bool admin;
    char* host;
    unsigned port;
    byte* serverSignPublicKey;
    unsigned serverSignPublicKeySize;
    unsigned usernameSize;
    unsigned passwordSize;
    char* credentials;
    OptionsHostIdSupplier hostIdSupplier;
    SDL_mutex* fileWriteGuard;
)
#pragma clang diagnostic pop

static char** nullable readOptionsFile(unsigned* linesCountBuffer) {
    SDL_RWops* rwOps = SDL_RWFromFile(OPTIONS_FILE, "r");
    if (!rwOps) return NULL;

    char buffer[MAX_FILE_SIZE];
    const unsigned actualFileSize = SDL_RWread(rwOps, buffer, 1, MAX_FILE_SIZE);
    SDL_RWclose(rwOps);
    if (!actualFileSize) return NULL;

    char** lines = NULL;
    unsigned linesCount = 0;

    char* line = NULL;
    for (unsigned i = 0, j = 0; i <= actualFileSize; i++) {
        line = SDL_realloc(line, ++j);

        if (buffer[i] == '\n' || i == actualFileSize) {
            line[j - 1] = 0;

            lines = SDL_realloc(lines, ++linesCount * sizeof(void*));
            lines[linesCount - 1] = line;

            j = 0;
            line = NULL;
        } else
            line[j - 1] = buffer[i];
    }

    *linesCountBuffer = linesCount;
    return lines;
}

static void parseAdminOption(const char* line)
{ this->admin = SDL_strstr(line, "true"); }

static void parseHostOption(const char* line) {
    const unsigned size = SDL_strlen(line),
        declarationSize = SDL_strlen(HOST_OPTION) + 1,
        payloadSize = size - declarationSize;

    this->host = SDL_malloc(payloadSize);
    SDL_memcpy(this->host, line + declarationSize, payloadSize);
}

static void parsePortOption(const char* line)
{ this->port = SDL_atoi(line + SDL_strlen(HOST_OPTION) + 1); }

static void parseSskpOption(const char* line) {
    const unsigned size = SDL_strlen(line);

    byte* nums = NULL;
    unsigned count = 0;
    char* num = NULL;

    for (unsigned i = SDL_strlen(SSPK_OPTION) + 1, j = 0; i <= size; i++) {
        num = SDL_realloc(num, ++j);

        if (line[i] == ',' || i == size) {
            num[j - 1] = 0;

            nums = SDL_realloc(nums, ++count);
            nums[count - 1] = (byte) SDL_atoi(num);

            SDL_free(num);
            num = NULL;
            j = 0;
        } else
            num[j - 1] = line[i];
    }

    this->serverSignPublicKey = nums;
    this->serverSignPublicKeySize = count;
}

static inline unsigned credentialsSize(void)
{ return this->usernameSize + this->passwordSize; }

static byte* makeKey(void) {
    const long hostId = (*(this->hostIdSupplier))();
    byte* key = SDL_malloc(CRYPTO_KEY_SIZE);

    for (unsigned i = 0; i < CRYPTO_KEY_SIZE; key[i] = ((byte*) &hostId)[i % sizeof(long)], i++);
    return key;
}

static void parseCredentialsOption(const char* line) {
    const int lineSize = (int) SDL_strlen(line),
        optionSize = (int) SDL_strlen(CREDENTIALS_OPTION) + 1,
        payloadSize = lineSize - optionSize;
    if (payloadSize <= 0) return;

    unsigned decodedSize = 0;
    byte* decoded = cryptoBase64Decode(line + optionSize, payloadSize, &decodedSize);
    if (!decoded) return;
    assert(decodedSize == cryptoSingleEncryptedSize(credentialsSize()));

    byte* key = makeKey();

    byte* decrypted = cryptoDecryptSingle(key, decoded, decodedSize);
    SDL_free(key);
    SDL_free(decoded);
    if (!decrypted) return;

    this->credentials = (char*) decrypted;
}

static bool createDefaultOptionsFileIfNotExists(void) {
    if (!access(OPTIONS_FILE, F_OK))
        return true;

    const unsigned size = 167;
    const byte content[size] = // hexdump -ve '1/1 "x%.2x"' options.txt | sed 's/x/\\x/g'
        "\x61\x64\x6d\x69\x6e\x3d\x74\x72\x75\x65\x0a\x68\x6f\x73\x74\x3d\x31\x32\x37\x2e\x30\x2e\x30\x2e\x31\x0a\x70\x6f"
        "\x72\x74\x3d\x38\x30\x38\x30\x0a\x73\x73\x70\x6b\x3d\x32\x35\x35\x2c\x32\x33\x2c\x32\x31\x2c\x32\x34\x33\x2c\x31"
        "\x34\x38\x2c\x31\x37\x37\x2c\x31\x38\x36\x2c\x30\x2c\x37\x33\x2c\x33\x34\x2c\x31\x37\x33\x2c\x31\x33\x30\x2c\x32"
        "\x33\x34\x2c\x32\x35\x31\x2c\x38\x33\x2c\x31\x33\x30\x2c\x31\x33\x38\x2c\x35\x34\x2c\x32\x31\x35\x2c\x35\x2c\x31"
        "\x37\x30\x2c\x31\x33\x39\x2c\x31\x37\x35\x2c\x31\x34\x38\x2c\x37\x31\x2c\x32\x31\x35\x2c\x37\x34\x2c\x31\x37\x32"
        "\x2c\x32\x37\x2c\x32\x32\x35\x2c\x32\x36\x2c\x32\x34\x39\x0a\x63\x72\x65\x64\x65\x6e\x74\x69\x61\x6c\x73\x3d";

    SDL_RWops* rwOps = SDL_RWFromFile(OPTIONS_FILE, "wb");
    if (!rwOps) return false;

    const unsigned written = SDL_RWwrite(rwOps, content, 1, size);

    SDL_RWclose(rwOps);
    return written == size;
}

bool optionsInit(unsigned usernameSize, unsigned passwordSize, OptionsHostIdSupplier hostIdSupplier) {
    assert(!this);
    this = SDL_malloc(sizeof *this);
    this->admin = false;
    this->host = NULL;
    this->port = 0;
    this->serverSignPublicKey = NULL;
    this->usernameSize = usernameSize;
    this->passwordSize = passwordSize;
    this->credentials = NULL;
    this->hostIdSupplier = hostIdSupplier;
    this->fileWriteGuard = NULL;

    assert(createDefaultOptionsFileIfNotExists());

    unsigned linesCount = 0;
    char** lines = readOptionsFile(&linesCount);
    if (!lines) {
        SDL_free(this);
        this = NULL;
        return false;
    }

    char* line;
    for (unsigned i = 0; i < linesCount; i++) {
        line = lines[i];

        if (SDL_strstr(line, ADMIN_OPTION)) parseAdminOption(line);
        else if (SDL_strstr(line, HOST_OPTION)) parseHostOption(line);
        else if (SDL_strstr(line, PORT_OPTION)) parsePortOption(line);
        else if (SDL_strstr(line, SSPK_OPTION)) parseSskpOption(line);
        else if (SDL_strstr(line, CREDENTIALS_OPTION)) parseCredentialsOption(line);
        else assert(false);

        SDL_free(line);
    }
    SDL_free(lines);

    this->fileWriteGuard = SDL_CreateMutex();
    return true;
}

bool optionsIsAdmin(void) {
    assert(this);
    return this->admin;
}

const char* optionsHost(void) {
    assert(this);
    return this->host;
}

unsigned optionsPort(void) {
    assert(this);
    return this->port;
}

const byte* optionsServerSignPublicKey(void) {
    assert(this);
    return this->serverSignPublicKey;
}

unsigned optionsServerSignPublicKeySize(void) {
    assert(this);
    return this->serverSignPublicKeySize;
}

const char* nullable optionsCredentials(void) {
    assert(this);
    return this->credentials;
}

static int findOffsetOfOptionPayload(const char* option) {
    SDL_RWops* rwOps = SDL_RWFromFile(OPTIONS_FILE, "r");
    if (!rwOps) return -1;

    char buffer[MAX_FILE_SIZE] = {0};
    const unsigned actualFileSize = SDL_RWread(rwOps, buffer, 1, MAX_FILE_SIZE);
    SDL_RWclose(rwOps);
    if (!actualFileSize) return -1;

    const char* optionStart = SDL_strstr(buffer, option) + SDL_strlen(option) + 1;
    if (!optionStart) return -1;

    return (int) ((unsigned long) optionStart - (unsigned long) buffer);
}

void optionsSetCredentials(const char* nullable credentials) {
    assert(this);

    const int offset = findOffsetOfOptionPayload(CREDENTIALS_OPTION);
    assert(offset > 0);

    if (!credentials) {
        SDL_LockMutex(this->fileWriteGuard);
        truncate(OPTIONS_FILE, offset);
        SDL_UnlockMutex(this->fileWriteGuard);
        return;
    }

    const unsigned size = credentialsSize();

    byte* key = makeKey();
    byte* encrypted = cryptoEncryptSingle(key, (const byte*) credentials, size);
    SDL_free(key);
    assert(encrypted);

    char* encoded = cryptoBase64Encode(encrypted, cryptoSingleEncryptedSize(size));
    SDL_free(encrypted);

    SDL_LockMutex(this->fileWriteGuard); {
        SDL_RWops* rwOps = SDL_RWFromFile(OPTIONS_FILE, "r+");
        assert(rwOps);

        SDL_RWseek(rwOps, offset, RW_SEEK_SET);
        SDL_RWwrite(rwOps, encoded, 1, SDL_strlen(encoded));

        SDL_RWclose(rwOps);
    } SDL_UnlockMutex(this->fileWriteGuard);

    SDL_free(encoded);
}

void optionsClear(void) {
    assert(this);

    if (this->credentials) cryptoFillWithRandomBytes((byte*) this->credentials, credentialsSize());

    SDL_free(this->host);
    SDL_free(this->serverSignPublicKey);
    SDL_free(this->credentials);
    SDL_DestroyMutex(this->fileWriteGuard);
    SDL_free(this);
}
