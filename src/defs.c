
#include "defs.h"

const char* APP_TITLE = "Exchatge";
const char* NET_HOST = "127.0.0.1";

const int APP_WIDTH = 9 * 50;
const int APP_HEIGHT = 16 * 50;
const int UI_UPDATE_PERIOD = 1000 / 60;
const int NET_UPDATE_PERIOD = 60 / 15;
const int NET_PORT = 8080;
const int NET_RECEIVE_BUFFER_SIZE = NET_MESSAGE_BODY_SIZE + sizeof(int) * 4 + sizeof(long); // 1028 bytes = 1 kb + 4 bytes
const int NET_MESSAGE_FLAG_SIZE = sizeof(int);
const int NET_MESSAGE_TIMESTAMP_SIZE = sizeof(long);
const int NET_MESSAGE_SIZE_SIZE = sizeof(int);
const int NET_MESSAGE_INDEX_SIZE = sizeof(int);
const int NET_MESSAGE_COUNT_SIZE = sizeof(int);
const int NET_FLAG_SERVER_PUBLIC_KEY = 0;
const int NET_FLAG_NONCE = 1;
const int NET_FLAG_HELLO_MESSAGE = 2;
const int NET_FLAG_MESSAGE = 3;
