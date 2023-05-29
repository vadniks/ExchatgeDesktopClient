
#include <sodium/sodium.h>
#include "crypto.h"

bool crInit() { return sodium_init() >= 0; }
