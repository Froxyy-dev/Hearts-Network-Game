#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "server/ServerCroupier.h"
#include "server/serwer-common.h"
#include "server/serwer-parser.h"
#include "err/err.h"

/// @brief Function setups server.
int setupServer(ServerArguments &serverArguments) {
    // Create a socket.
    int socketFd = socket(AF_INET6, SOCK_STREAM, 0);
    if (socketFd < 0) {
        sysFatal("cannot create a socket");
    }

    // Bind the socket to a concrete address.
    sockaddr_in6 serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin6_family = AF_INET6;  // IPv6
    serverAddress.sin6_addr = in6addr_any; // Listening on all interfaces.
    serverAddress.sin6_port = htons(serverArguments.port);

    if (bind(socketFd, (sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        sysFatal("bind");
    }

    // Switch the socket to listening.
    if (listen(socketFd, ServerConstants::QUEUE_LENGTH) < 0) {
        sysFatal("listen");
    }

    return socketFd;
}

int main(int argc, char **argv) {
    ServerArguments serverArguments = ServerArguments();
    ServerStatus serverStatus;
    parseUserInput(argc, argv, serverArguments, serverStatus);

    int socketFd = setupServer(serverArguments);

    ServerCroupier serverCroupier = ServerCroupier(socketFd, serverArguments, serverStatus);
    serverCroupier.handleGame();

    close(socketFd);

    return 0;
}