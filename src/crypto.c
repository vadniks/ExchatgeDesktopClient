
#include <sdl/SDL.h>
#include <sodium/sodium.h>
#include "crypto.h"

typedef struct {
    byte* publicKey;
} this_t;

static this_t* this = NULL;

bool crInit(byte* publicKey) {
    if (sodium_init() < 0) return false;

    this = SDL_malloc(sizeof *this);
    this->publicKey = publicKey;

    return true;
}

void crGenerateKeypair() {

}

byte* crEncrypt(byte* bytes) {
    return NULL;
}

byte* crDecrypt(byte* bytes) {
    return NULL;
}

void crClean() {
    SDL_free(this);
}
