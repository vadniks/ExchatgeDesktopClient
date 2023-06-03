
#include "lifecycle.h"

#include "crypto.h" // TODO: test only
#include <stdlib.h>
#include <sdl/SDL.h>

int main() {
    crCryptDetails* cryptDetails = SDL_malloc(sizeof *cryptDetails); // TODO: test only
    cryptDetails->blockSize = 16;
    cryptDetails->unpaddedSize = NET_MESSAGE_SIZE; // 1048
    cryptDetails->paddedSize = 1056;
    crInit(NULL, cryptDetails);
    crClean();

//    if (!lcInit()) return 1;
//    lcLoop();
//    lcClean();
    return 0;
}
