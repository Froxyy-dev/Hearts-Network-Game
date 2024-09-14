#ifndef KIERKI_SERVERCROUPIER_H
#define KIERKI_SERVERCROUPIER_H

#include <arpa/inet.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <fcntl.h>
#include <fstream>
#include <sstream>

#include "server/ServerContext.h"
#include "server/serwer-common.h"
#include "server/serwer-communicator.h"
#include "common/common.h"
#include "err/err.h"

class ServerCroupier {
  private:
    ServerStatus serverStatus;
    ServerContext serverContext;

    /// HELPER FUNCTIONS ///

    /// @brief Function for closing connection with a player.
    void closeConnectionWithPlayer(int index);

    /// @brief Function returns free slot if found and ERROR_CODE otherwise.
    int findFreeSlot();

    /// HELPER FUNCTIONS FOR SENDING MESSAGES ///

    /// @brief Function to be called before sending wrong.
    void prepareSendingWrong(int index);

    /// @brief Function to be called before sending busy.
    void prepareSendingBusy(int index);

    /// @brief Function to be called before sending deal.
    void prepareSendingDeal(int index, CLIENT_STATE clientState);

    /// @brief Function to be called after sending deal.
    void afterSendingDeal(int index);

    /// @brief Function to be called before sending trick to client at given index.
    void prepareSendingTrick(int index);

    /// @brief Function to be called after sending trick to client at given index.
    void afterSendingTrick(int index);

    /// @brief Function to be called before sending taken to clients.
    void prepareSendingTaken();

    /// @brief Function to be called after sending taken message.
    void afterSendingTaken(int index);

    /// @brief Function to be called after sending deal/taken message.
    void afterSendingPrevious(int index);

    /// @brief Function to be called before sending score.
    void prepareSendingScore(int index);

    /// @brief Function to be called after sending score.
    void afterSendingScore(int index);

    /// @brief Function to be called before sending total.
    void prepareSendingTotal(int index);

    /// @brief Function to be called after sending total.
    void afterSendingTotal(int index);

    /// @brief Function starts to close waiting clients because of game start.
    void startClosingWaiting();

    /// FUNCTIONS FOR HANDLING MESSAGES FROM CLIENTS ///

    /// @brief Returns client place on success and TABLE_PLACE::UNDEFINED otherwise.
    TABLE_PLACE handleNonPlayerBuffer(int index);

    /// @brief Handles messages from buffer and sets ClientStatus accordingly.
    void handleNonPlayerMessage(int index);

    /// @brief Handles message when not expecting it.
    void handleNonCurrentMessage(int index);

    /// @brief Function to be called after receiving trick.
    void afterReceivingTrick(int index);

    /// @brief Handles message from player when we waited for TRICK.
    void handleCurrentMessage(int index);

    /// @brief We handle players buffer.
    void handlePlayersBuffer();

    /// FUNCTION FOR ACCEPTING NEW CONNECTION

    /// @brief Function handles new connection.
    void handleNewConnection();

    /// FUNCTION FOR HANDLING TIMEOUT ///

    /// @brief Function for handling timeout.
    void handleTimeout();

    /// FUNCTIONS FOR READING MESSAGES ///

    /// @brief Handling read of non player.
    void readFromNonPlayer(int index, char *buffer);

    /// @brief Function reads messages from non players.
    void readFromNonPlayers(char *buffer);

    /// @brief Function reads messages from players.
    void readFromPlayers(char *buffer);

    /// FUNCTIONS FOR SENDING MESSAGES ///

    /// @brief Returns true if server sent busy message.
    bool serverSentBusy(int index, ssize_t sentLen);

    /// @brief Function writes to non players.
    void writeToNonPlayers();

    /// @brief Function writes to players if poll is including them.
    void writeToPlayers();

    /// CONSTRUCTOR FUNCTION ///
  public:
    ServerCroupier(int socketFd, ServerArguments &serverArguments, ServerStatus &serverStatus);

    /// @brief Server handles game.
    void handleGame();
};

#endif // KIERKI_SERVERCROUPIER_H
