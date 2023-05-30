
#include "defs.h"

const char* APP_TITLE = "Exchatge";

const int APP_WIDTH = 9 * 50;
const int APP_HEIGHT = 16 * 50;
const int UI_UPDATE_PERIOD = 1000 / 60;
const int NET_UPDATE_PERIOD = 60 / 15;
const int NET_RECEIVE_BUFFER_SIZE = (1 << 10) + 2 * 4; // 1032 bytes = 1 kb + 2 * 4 bytes
const int NET_MESSAGE_HEAD_SIZE = 4; // TODO: correct & add new constants, rename to packet
const int NET_MESSAGE_TAIL_SIZE = 4;

/*
Here's the message protocol:
A message may consist of several packets (for file transfer).
packet:
    header:
        8 bytes long timestamp in milliseconds from the epoch (ulong64);
        4 bytes long payload size (uint32) (a message payload may be from 1 to 1024 bytes long);
        4 bytes long packet index (int32) (negative if a message consists of one packet);
        4 bytes long total packets count (uint32) that a current message occupies;
        4 bytes long service information specifier (flags);
    body:
        1024 bytes long message;
    tail:
        4 bytes long checksum // TODO: choose fast checksum function
*/

// TODO: add a local storage to cache messages
