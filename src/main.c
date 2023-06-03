
#include "lifecycle.h"

int main() {
    if (!lifecycleInit()) return 1;
    lifecycleLoop();
    lifecycleClean();
    return 0;
}
