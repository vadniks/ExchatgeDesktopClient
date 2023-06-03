
#include "defs.h"

const char* APP_TITLE = "Exchatge";
const char* NET_HOST = "127.0.0.1";

const unsigned APP_WIDTH = 9 * 50;
const unsigned APP_HEIGHT = 16 * 50;
const unsigned UI_UPDATE_PERIOD = 1000 / 60;
const unsigned NET_UPDATE_PERIOD = 60 / 15;

staticAssert(sizeof(char) == 1 && sizeof(int) == 4 && sizeof(long) == 8);
