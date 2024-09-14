#include <arpa/inet.h>
#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sstream>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "client/ClientPlayer.h"
#include "client/klient-common.h"
#include "client/klient-parser.h"
#include "common/common.h"
#include "err/err.h"

struct addrinfo *getAddrinfo(const char *host, ClientArguments &clientArguments) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = clientArguments.aiFamily;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *addressResult;
    int errcode = getaddrinfo(host, nullptr, &hints, &addressResult);
    if (errcode != 0) {
        sysFatal("getaddrinfo: %s", gai_strerror(errcode));
    }

    return addressResult;
}

int main(int argc, char **argv) {
    ClientArguments clientArguments = ClientArguments();
    parseUserInput(argc, argv, clientArguments);

    const char *host = clientArguments.host;
    uint16_t port = readPort(clientArguments.port);

    struct addrinfo *addressResult = getAddrinfo(host, clientArguments);
    int aiFamily = addressResult->ai_family;

    int socketFd = socket(aiFamily, SOCK_STREAM, 0);
    if (socketFd < 0) {
        sysFatal("cannot create a socket");
    }

    std::string serverAddressStr;

    if (aiFamily == AF_INET) {
        struct sockaddr_in serverAddressIpv4;
        setServerAddressIpv4(port, addressResult, &serverAddressIpv4);

        if (connect(socketFd, (struct sockaddr *)&serverAddressIpv4,
                    (socklen_t)sizeof(serverAddressIpv4)) < 0) {
            sysFatal("cannot connect to the server");
        }

        serverAddressStr = getIpv4AndPortAddress(serverAddressIpv4);
    } else {
        struct sockaddr_in6 serverAddressIpv6;
        setServerAddressIpv6(port, addressResult, &serverAddressIpv6);

        if (connect(socketFd, (struct sockaddr *)&serverAddressIpv6,
                    (socklen_t)sizeof(serverAddressIpv6)) < 0) {
            sysFatal("cannot connect to the server");
        }

        serverAddressStr = getIpv6AndPortAddress(serverAddressIpv6);
    }

    std::string clientAddressStr;

    if (aiFamily == AF_INET) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        memset(&clientAddress, 0, sizeof(clientAddressLen));

        if (getsockname(socketFd, (struct sockaddr *)&clientAddress, &clientAddressLen) == -1) {
            sysFatal("getsockname");
        }

        clientAddressStr = getIpv4AndPortAddress(clientAddress);
    } else {
        struct sockaddr_in6 clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        memset(&clientAddress, 0, sizeof(clientAddressLen));

        if (getsockname(socketFd, (struct sockaddr *)&clientAddress, &clientAddressLen) == -1) {
            sysFatal("getsockname");
        }

        clientAddressStr = getIpv6AndPortAddress(clientAddress);
    }

    if (fcntl(socketFd, F_SETFL, O_NONBLOCK)) {
        sysFatal("fcntl");
    }

    ClientPlayer clientPlayer =
        ClientPlayer(socketFd, clientArguments, serverAddressStr, clientAddressStr);
    clientPlayer.handleGame();

    close(socketFd);

    return 0;
}