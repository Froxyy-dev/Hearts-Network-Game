#ifndef KIERKI_SERVERCONTEXT_H
#define KIERKI_SERVERCONTEXT_H

#include <unistd.h>

#include "common.h"
#include "serwer-common.h"

class ServerContext {
  private:
    int baseTimeout;
    std::vector<int> socketTimeouts;
    std::vector<bool> waitingFor;
    std::vector<std::string> clientAddressStr;
    std::vector<std::string> serverAddressStr;

    int socketFd;
    pollfd pollDescriptors[ServerConstants::CONNECTIONS];

    std::vector<short> storedPollEvents;
    std::vector<int> storedIndexes;
    std::vector<ReadBuffer> readBuffers;
    std::vector<WriteBuffer> writeBuffers;
    std::vector<CLIENT_STATE> clientStates;

  public:
    void createContext(int _baseTimeout, int _socketFd);

    /// FUNCTIONS RESPONSIBLE FOR POLL DESCRIPTORS. ///

    /// @brief Function initializes poll structures.
    void initializePollStructures();

    /// @brief Function resets poll descriptor at given index.
    void resetPollDescriptor(int index);

    /// @brief Returns true if descriptor at given index is reserved and false otherwise.
    bool isDescriptorReserved(int index);

    /// @brief Function closes descriptor at given index.
    void closeDescriptor(int index);

    /// @brief Returns descriptor at given index.
    int getPollDescriptor(int index);

    /// @brief Returns true if there is data to read at given descriptor and false otherwise.
    bool pollReadAt(int index);

    /// @brief Returns true if server can write to given descriptor and false otherwise.
    bool pollWriteAt(int index);

    /// @brief Functions sets events to check if server can write at given index.
    void pollSetWrite(int index);

    /// FUNCTIONS RESPONSIBLE FOR POLL. ///

    /// @brief Functions returns minimal timeout or -1 if server is not waiting for any client.
    int getPollTimeout(int startingPoint);

    /// @brief Function resets revents starting from startingPoint for non empty descriptors.
    void resetRevents(int startingPoint);

    /// @brief Functions subtracts time from socket timeouts for clients server is waiting for.
    void revaluateTimeouts(int duration, int startingPoint);

    /// @brief Function for executing poll.
    int executePoll(bool includePlayers);

    /// FUNCTIONS FOR HANDLING TIMEOUTS. ///

    /// @brief Sets flag to start waiting for descriptor at given index.
    void startWaitingFor(int index);

    /// @brief Sets flag to stop waiting for descriptor at given index.
    void stopWaitingFor(int index);

    /// @brief Functions returns true if socket[index] timed out and false otherwise.
    bool timeoutAt(int index);

    /// @brief Function resets timeout for socket at given index.
    void resetTimeout(int index);

    /// FUNCTIONS FOR HANDLING SERVER_CONNECTIONS. ///

    /// @brief Function accepts connection from new client. If game is full then sets status to
    /// sending busy and puts busy message in buffer, otherwise sets status to waiting for IAM
    /// and starts timeout.
    void acceptConnection(int index, int clientFd, bool gameFull, const std::string &clientIP,
                          const std::string &serverIp);

    /// @brief Functions moves client to players place.
    void movePlayer(int from, int to);

    /// @brief Function closes connection.
    void closeConnection(int index, bool closeFd);

    /// FUNCTIONS FOR DISPLAYING MESSAGES. ///

    /// @brief Function displays message from client at given index to server.
    void displayMessageFromClient(int index, const std::string &message);

    /// @brief Function displays message from server to client at given index.
    void displayMessageFromServer(int index, const std::string &message);

    /// @brief Function sends message to descriptor at given index.
    ssize_t sendMessageServer(int index, std::string &message);

    /// FUNCTIONS FOR HANDLING BUFFERS. ///

    /// @brief Returns true if there is message at given index.
    bool hasMessageFrom(int index);

    /// @brief If write buffer is empty then it sets descriptor events to read only.
    void checkIfEmpty(int index);

    /// @brief Function initiates sending message to client and sets his status accordingly.
    void initiateSending(int index, std::string message, CLIENT_STATE clientState);

    /// @brief Functions returns true if we have sent previous to everyone.
    bool hasEveryoneReceivedPreviousTaken();

    /// @brief Returns true if index is in indexes vector.
    bool inIndexes(const std::vector<int> &indexes, int index);

    /// @brief Storing events for sending previous deal and taken to new player.
    void storeEventsExceptIndexes(std::vector<int> indexes);

    /// @brief Restoring events after sending previous deal and taken to new player.
    void restoreEventsExceptIndexes();

    /// @brief Sets client state at given index.
    void setClientStateAt(int index, CLIENT_STATE clientState);

    /// @brief Returns client state at given index.
    CLIENT_STATE getClientStateAt(int index);

    /// @brief Returns current write message at given index.
    std::string getCurrentWriteMessageAt(int index);

    /// @brief Returns first write message at given index.
    std::string getFirstWriteMessageAt(int index);

    ///@brief Appends message to write buffer at given index.
    void appendMessageToWriteAt(int index, std::string message);

    /// @brief Calls wroteWholeMessage function from write buffer.
    bool wroteWholeMessageAt(int index, int sentLen);

    /// @brief Pops and returns first message at given index.
    std::string popFirstReadMessageAt(int index);

    ///@brief Appends message to read buffer at given index.
    void appendMessageToReadAt(int index, std::string message);
};

#endif // KIERKI_SERVERCONTEXT_H
