
#include <stdlib.h>
#include "lifecycle.h"

int main(int argc, const char** argv) {
    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
    lifecycleLoop();
    lifecycleClean();
    return EXIT_SUCCESS;
}
