
#include "lifecycle.h"
#include <sodium/sodium.h> // TODO: test only
#include <stdio.h>

int main() {
    sodium_init();

    // whole message size = head + body = 24 + 1024 = 1048 so we need a 1088 sized receive buffer (1048 + mac + nonce = 1048 + 16 + 24)
    unsigned char buf[1088] = {0}; // TODO: test only
    unsigned long unpaddedLen = 1072;
    for (unsigned i = 0; i < 256; i++) buf[i] = i;
    unsigned long paddedLen = 0;
    unsigned long blockLen = 16;

    for (unsigned i = 0; i < 256; i++) printf("%u ", buf[i]);
    printf("\n");

    int res = sodium_pad(&paddedLen, buf, unpaddedLen, blockLen, 1088);
    printf("%d %lu\n", res, paddedLen);
    for (unsigned i = 0; i < paddedLen; i++) printf("%u_", buf[i]);
    printf(" #\n");

    int res2 = sodium_unpad(&unpaddedLen, buf, paddedLen, blockLen);
    printf("%d\n", res2);
    for (unsigned i = 0; i < 256; i++) printf("%u ", buf[i]);
    printf("\n");

    // TODO: maximum message length is 1072 to fit it in the 1088 long buffer with padding applied

//    if (!lcInit()) return 1;
//    lcLoop();
//    lcClean();
    return 0;
}
