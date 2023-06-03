
#include "lifecycle.h"

#include "crypto.h" // TODO: test only
#include <stdlib.h>
#include <sdl/SDL.h>
#include <sodium/sodium.h>

int main() {
    int n = 32, d = 16, n2;

    if (n == 0) n2 = 0;
    else n2 = n + 1;

//    if (n > 1) n2 = n - 1;
//    else if (n == 1) n2 = 1;
//    else n2 = 0;

    div_t r = div(n2, d);
    printf("%d %d | %d\n", r.quot, r.rem, d * (r.quot + (r.rem > 0 ? 1 : 0)));

    unsigned char buf[100];
    size_t        buf_unpadded_len = 32;
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
//    cryptDetails->unpaddedSize = 1048;
//    cryptDetails->paddedSize = 1056;
//    crInit(NULL, cryptDetails);
//    crClean();

//    if (!lcInit()) return 1;
//    lcLoop();
//    lcClean();
    return 0;
}
