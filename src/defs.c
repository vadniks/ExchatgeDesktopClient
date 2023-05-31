
#include "defs.h"

const char* APP_TITLE = "Exchatge";
const char* NET_HOST = "127.0.0.1";

const int APP_WIDTH = 9 * 50;
const int APP_HEIGHT = 16 * 50;
const int UI_UPDATE_PERIOD = 1000 / 60;
const int NET_UPDATE_PERIOD = 60 / 15;
const int NET_PORT = 8080;
const int NET_RECEIVE_BUFFER_SIZE = (1 << 10) + 2 * 4; // 1028 bytes = 1 kb + 4 bytes
const int NET_MESSAGE_HEAD_SIZE = 4;
const int NET_MESSAGE_BODY_SIZE = 4;
