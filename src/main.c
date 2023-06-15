
#include <stdlib.h>
#include "lifecycle.h"

int main(void) {
    if (!lifecycleInit()) return EXIT_FAILURE;
    lifecycleLoop();
    lifecycleClean();
    return EXIT_SUCCESS;
}
