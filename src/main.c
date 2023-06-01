
#include "lifecycle.h"

int main() {
    if (!lcInit()) return 1;
    lcLoop();
    lcClean();
    return 0;
}
