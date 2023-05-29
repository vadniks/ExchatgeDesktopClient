
#include <sodium/sodium.h>
#include "lifecycle.h"

int main() {
    if (sodium_init() < 0) perror("unable to init sodium lib");

    lcInit();
    lcLoop();
    lcClean();
    return 0;
}
