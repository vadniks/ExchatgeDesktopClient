
#include "defs.h"

const char* APP_TITLE = "Exchatge";
const char* NET_HOST = "127.0.0.1";

const int APP_WIDTH = 9 * 50;
const int APP_HEIGHT = 16 * 50;
const int UI_UPDATE_PERIOD = 1000 / 60;
const int NET_UPDATE_PERIOD = 60 / 15;
const int NET_PORT = 8080;
const int NET_MESSAGE_HEAD_SIZE = sizeof(int) * 4 + sizeof(long);
const int NET_MESSAGE_SIZE = NET_MESSAGE_HEAD_SIZE + NET_MESSAGE_BODY_SIZE;
const int NET_RECEIVE_BUFFER_SIZE = 1056;
const int NET_FLAG_UNAUTHENTICATED = 0x7ffffffe;
const int NET_FLAG_FINISH = 0x7fffffff;

staticAssert(sizeof(char) == 1 && sizeof(int) == 4 && sizeof(long) == 8);
