
#include "lifecycle.h"

#include "crypto.h" // TODO: test only
#include <stdlib.h>
#include <sdl/SDL.h>

int main() {
    byte* buf = SDL_calloc(10, 1);
    SDL_memset(buf, 'a', 5);
    SDL_memset(buf + 5u, 'b', 5);
    SDL_free(buf);

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
