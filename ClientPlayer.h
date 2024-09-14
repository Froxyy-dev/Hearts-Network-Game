#ifndef KIERKI_CLIENTPLAYER_H
#define KIERKI_CLIENTPLAYER_H

#include <arpa/inet.h>
#include <chrono>
#include <ctype.h>
#include <endian.h>
#include <fcntl.h>
#include <iomanip>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ClientContext.h"
#include "common.h"
#include "err.h"
#include "klient-common.h"
#include "klient-communicator.h"

class ClientPlayer {
  private:
    ClientArguments clientArguments;
    std::string serverAddressStr;
    std::string clientAddressStr;
    ClientContext clientContext;

    /// @brief Clients sends iam message.
    void clientInitiate();

    /// @brief Function handles receiving busy.
    void receiveBusy(std::string serverMessage);

    /// @brief Function handles receiving deal.
    void receiveDeal(std::string serverMessage);

    /// @brief Function handles receiving trick.
    void receiveTrick(std::string serverMessage);

    /// @brief Function handles receiving taken.
    void receiveTaken(std::string serverMessage);

    /// @brief Function handles receiving wrong.
    void receiveWrong(std::string serverMessage);

    /// @brief Function handles receiving score.
    void receiveScore(std::string serverMessage);

    /// @brief Function handles receiving total.
    void receiveTotal(std::string serverMessage);

    /// @brief Handles message from server.
    void handleMessageFromServer(std::string serverMessage);

    /// @brief Handles message from user.
    void handleMessageFromUser(const std::string &userMessage);

    /// @brief Client reads message from user.
    void pollFromUser(char *buffer);

    /// @brief Client reads message from server.
    int pollFromServer(char *buffer);

    /// @brief Client writes to server.
    void writeToServer();

  public:
    ClientPlayer(int _socketFd, const ClientArguments &_clientArguments,
                 const std::string &_serverAddressStr, const std::string &_clientAddressStr);

    /// @brief Client handles game.
    void handleGame();
};

#endif // KIERKI_CLIENTPLAYER_H
