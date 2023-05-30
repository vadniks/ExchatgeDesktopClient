
#include "defs.h"

const char* APP_TITLE = "Exchatge";

const int APP_WIDTH = 9 * 50;
const int APP_HEIGHT = 16 * 50;
const int UI_UPDATE_PERIOD = 1000 / 60;
const int NET_UPDATE_PERIOD = 60 / 15;
const int NET_RECEIVE_BUFFER_SIZE = (1 << 10) + 2 * 4; // 1032 bytes = 1 kb + 2 * 4 bytes
const int NET_MESSAGE_HEAD_SIZE = 4;
const int NET_MESSAGE_TAIL_SIZE = 4; // TODO: add hash sum

/*
message:
    4-bytes long header containing an actual message size (uint32);
    1024-bytes long body for a message;
    4-bytes long tail for a service information specifier
*/
