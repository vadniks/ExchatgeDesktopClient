
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void error(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(__attribute_maybe_unused__ int argc, __attribute_maybe_unused__ char** argv) {
    int descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor < 0) error("unable to create a socket");

    struct sockaddr_in address = { AF_INET, htons(8080) };
    if (inet_pton(AF_INET, "127.0.0.1", &(address.sin_addr)) <= 0) error("invalid address");

    int status = connect(descriptor, (struct sockaddr*) &address, sizeof address);
    if (status < 0) error("connection failed");

    send(descriptor, "Hello", 5, 0);

    char buffer[256] = { 0 };
    read(descriptor, buffer, 256);
    printf("%s\n", buffer);

    return EXIT_SUCCESS;
}
