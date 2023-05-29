
#include <sdl_net/SDL_net.h>
#include "net.h"

char* net() {
    IPaddress address;
    SDLNet_ResolveHost(&address, "127.0.0.1", 8080);

    char* buffer = NULL;

    TCPsocket socket = SDLNet_TCP_OpenClient(&address);
    if (!socket) goto end;

    SDLNet_TCP_Send(socket, "Hello", 5);

    buffer = SDL_calloc(256, sizeof(char));
    SDLNet_TCP_Recv(socket, buffer, 256);
    SDL_Log("%s\n", buffer);

    end:
    SDLNet_TCP_Close(socket);
    return buffer;
}
