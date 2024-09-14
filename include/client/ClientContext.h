#ifndef KIERKI_CLIENTCONTEXT_H
#define KIERKI_CLIENTCONTEXT_H

#include "common/common.h"
#include "klient-common.h"

class ClientContext {
  private:
    int socketFd;
    pollfd pollDescriptors[ClientConstants::CLIENT_CONNECTIONS];

    std::string clientAddressStr;
    std::string serverAddressStr;

    ReadBuffer userBuffer;
    ReadBuffer serverBuffer;
    WriteBuffer writeBuffer;

    std::vector<std::vector<Card>> takenTricks;
    ClientHand clientHand;

    bool isAutomatic;

  public:
    void createContext(const std::string &_clientAddressStr, const std::string &_serverAddressStr,
                       int _socketFd, bool _isAutomatic, TABLE_PLACE _tablePlace);

    /// @brief Returns true if game can be finished.
    bool isGameFinished();

    /// @brief Initializes poll structures.
    void initializePollStructures();

    /// @brief Returns true if client has not sent trick yet.
    bool clientCanSendTrick();

    /// @brief Resets revents.
    void resetRevents();

    /// @brief Checks if there is data to read from user.
    bool pollReadFromUser();

    /// @Brief Checks if there is data to read from server.
    bool pollReadFromServer();

    /// @brief Checks if you can write to server.
    bool pollWriteToServer();

    /// @brief Displays message from server if client is automatic.
    void displayMessageFromServer(const std::string &message);

    /// @brief Displays message from client if client is automatic.
    void displayMessageFromClient(const std::string &message);

    /// @brief Displays card vector without ending dot.
    void displayCards();

    /// @brief Displays trick which client has taken.
    void displayTricks();

    /// @brief Client sends message to server.
    ssize_t sendMessageClient(std::string &message);

    /// @brief Function initiates sending message to client and sets his status accordingly.
    void initiateSending(std::string message);

    /// @brief Sets players cards.
    void setPlayerCards(std::vector<Card> cards);

    /// @brief Sets hand type.
    void setHandType(HAND_TYPE handType);

    /// @brief Sets previous trick taker.
    void setPreviousTrickTaker(TABLE_PLACE tablePlace);

    /// @brief Function to be called after taken.
    void afterTaken(TABLE_PLACE takesTrick, int cardIndex);

    /// @brief Returns true if client has taken last trick.
    bool tookLastTrick();

    /// @brief Function to be called after receiving deal.
    void afterReceivingDeal();

    /// @brief Function to be called after receiving trick.
    void afterReceivingTrick();

    /// @brief Function to be called after receiving taken.
    void afterReceivingTaken();

    /// @brief Function to be called after receiving score.
    void afterReceivingScore();

    /// @brief Function to be called after receiving total.
    void afterReceivingTotal();

    /// @brief Returns true if client is automatic.
    bool isClientAutomatic();

    /// @brief Function to be called after deal.
    void clearTakenTricks();

    /// @brief Appends taken trick.
    void appendTakenTrick(std::vector<Card> trickCards);

    /// @brief Returns poll descriptor at given index.
    int getPollDescriptorAt(int index);

    /// @brief Function executes poll.
    int executePoll();

    /// @brief Function starts waiting for read at given index.
    void setReadAt(int index);

    /// @brief Appends current read to user buffer.
    void appendUserRead(std::string message);

    /// @brief Returns true if there is any message from user.
    bool hasUserMessage();

    /// @brief Pops first user message.
    std::string popFirstUserMessage();

    /// @brief Appends current read to server buffer.
    void appendServerRead(std::string message);

    /// @brief Returns true if there is any message from server.
    bool hasServerMessage();

    /// @brief Pops first server message.
    std::string popFirstServerMessage();

    /// @brief Returns true if buffer has write message.
    bool hasWriteMessage();

    /// @brief Returns first write message.
    std::string getFirstWriteMessage();

    /// @brief Returns current write message.
    std::string getCurrentWriteMessage();

    /// @brief Calls wrote whole message function in write buffer.
    bool wroteWholeWriteMessage(int sentLen);

    /// @brief Returns client hand.
    ClientHand getClientHand();

    /// @brief Client sent trick.
    void setSentTrickTo(bool value);
};

#endif // KIERKI_CLIENTCONTEXT_H
