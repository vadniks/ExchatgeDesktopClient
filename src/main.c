
#include "lifecycle.h"

#include "crypto.h" // TODO: test only
#include <stdlib.h>
#include <sdl/SDL.h>
#include <sodium/sodium.h>

int main() {
    crCryptDetails* cryptDetails = SDL_malloc(sizeof *cryptDetails); // TODO: test only
    cryptDetails->blockSize = 16;
    cryptDetails->unpaddedSize = 1048;
    cryptDetails->paddedSize = 1056;
    crInit(NULL, cryptDetails);
    crClean();

//    if (!lcInit()) return 1;
//    lcLoop();
//    lcClean();
    return 0;
}
