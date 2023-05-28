
#include <stdio.h>
#include <sdl/SDL.h>
#include <sdl_net/SDL_net.h>
//#include "nuklear.h" // TODO

#define ERROR(x) { perror(#x); return EXIT_FAILURE; }

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER)) ERROR(unable to init sdl)
    if (SDLNet_Init()) ERROR(unable to init sdl_net)

    IPaddress address;
    if (SDLNet_ResolveHost(&address, "127.0.0.1", 8080)) ERROR(unable to resolve host)

    TCPsocket socket = SDLNet_TCP_OpenClient(&address);
    if (!socket) ERROR(unable to connect)

    SDLNet_TCP_Send(socket, "Hello", 5);

    char buffer[256] = { 0 };
    SDLNet_TCP_Recv(socket, buffer, 256);
    printf("%s\n", buffer);

    SDLNet_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
