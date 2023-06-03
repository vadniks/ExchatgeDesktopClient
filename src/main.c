
#include "lifecycle.h"

#include "crypto.h" // TODO: test only
#include <stdlib.h>
#include <sdl/SDL.h>
#include <sodium/sodium.h>

int main() {
    unsigned char buf[2048];
    size_t        buf_unpadded_len = 1048; // TODO: too many sizes... So, message length is 1048, we add padding to it so it's length becomes 1056, finally we encrypt the result, which length becomes 1096 (padded + mac + nonce)
    size_t        buf_padded_len;
    size_t        block_size = 16;

/* round the length of the buffer to a multiple of `block_size` by appending
 * padding data and put the new, total length into `buf_padded_len` */
    if (sodium_pad(&buf_padded_len, buf, buf_unpadded_len, block_size, sizeof buf) != 0) {
        /* overflow! buf[] is not large enough */
    }

/* compute the original, unpadded length */
    if (sodium_unpad(&buf_unpadded_len, buf, buf_padded_len, block_size) != 0) {
        /* incorrect padding */
    }

    printf("%zu\n", buf_padded_len);

//    crCryptDetails* cryptDetails = SDL_malloc(sizeof *cryptDetails); // TODO: test only
//    cryptDetails->blockSize = 16;
//    cryptDetails->unpaddedSize = NET_MESSAGE_SIZE; // 1048
//    cryptDetails->paddedSize = 1056;
//    crInit(NULL, cryptDetails);
//    crClean();

//    if (!lcInit()) return 1;
//    lcLoop();
//    lcClean();
    return 0;
}
