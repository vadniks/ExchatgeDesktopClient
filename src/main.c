
#include "lifecycle.h"
#include <sodium/sodium.h> // TODO: test only
#include <stdio.h>

int main() {
    sodium_init();

    unsigned char buf[1024] = {0}; // TODO: test only
    unsigned long unpaddedLen = 1008;
    for (unsigned i = 0; i < 256; i++) buf[i] = i;
    unsigned long paddedLen = 0;
    unsigned long blockLen = 16;

    for (unsigned i = 0; i < 256; i++) printf("%u ", buf[i]);
    printf("\n");

    int res = sodium_pad(&paddedLen, buf, unpaddedLen, blockLen, 1024);
    printf("%d %lu\n", res, paddedLen);
    for (unsigned i = 0; i < paddedLen; i++) printf("%u_", buf[i]);
    printf(" #\n");

    int res2 = sodium_unpad(&unpaddedLen, buf, paddedLen, blockLen);
    printf("%d\n", res2);
    for (unsigned i = 0; i < 256; i++) printf("%u ", buf[i]);
    printf("\n");

    // TODO: maximum message length is 1008 to fit it in the 1024 long buffer with padding applied

//    if (!lcInit()) return 1;
//    lcLoop();
//    lcClean();
    return 0;
}
