#ifndef KIERKI_COMMON_H
#define KIERKI_COMMON_H

#include <arpa/inet.h>
#include <chrono>
#include <errno.h>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <queue>
#include <signal.h>
#include <sstream>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "err.h"

/// NAMESPACES ///

namespace Constants {
const int ERROR_CODE = -1;
const int TRICK_NUMBER = 13;
const int PLAYERS_NUMBER = 4;
const int CARDS_NUMBER = 13;
const std::string AVAILABLE_COMMANDS = "Available commands: (!{card} / tricks / cards) + enter\n";
} // namespace Constants

namespace Messages {
const std::string IAM = "IAM";
const std::string BUSY = "BUSY";
const std::string DEAL = "DEAL";
const std::string TRICK = "TRICK";
const std::string WRONG = "WRONG";
const std::string TAKEN = "TAKEN";
const std::string SCORE = "SCORE";
const std::string TOTAL = "TOTAL";
const std::string END_OF_MESSAGE = "\r\n";
} // namespace Messages

/// ENUMS ///

enum class TABLE_PLACE { N = 0, E = 1, S = 2, W = 3, UNDEFINED = -1 };

enum class HAND_TYPE {
    DEFAULT = 1,
    HEART = 2,
    QUEEN = 3,
    GUYS = 4,
    HEART_KING = 5,
    SEVEN_N_LAST = 6,
    BANDIT = 7,
    UNDEFINED = -1
};

enum class CARD_COLOR { C = 0, D = 1, H = 2, S = 3, UNDEFINED = -1 };

enum class CARD_VALUE { J = 11, Q = 12, K = 13, A = 14, UNDEFINED = -1 };

enum class CLIENT_STATE {
    WAITING_FOR_TURN,
    SENDING_BUSY,
    WAITING_FOR_DEAL,
    SENDING_DEAL,
    SENDING_TRICK,
    WAITING_FOR_TRICK,
    SENDING_TAKEN,
    SENDING_SCORE,
    SENDING_TOTAL,
    WAITING_FOR_START,
    SENDING_WRONG,
    WAITING_FOR_IAM,
    SENDING_PREVIOUS,
    EMPTY_PLACE,
};

/// STRUCTS ///

struct ReadBuffer {
    std::string buffer;

    ReadBuffer() {
        buffer = "";
    }

    /// @brief Append message to buffer.
    void appendRead(std::string message) {
        buffer += message;
    }

    /// @brief Returns length of a message ending with \ r\ n.
    int networkMessageLen() {
        if (buffer.size() < 2) {
            return 0;
        }

        int messageLen = 0;
        for (size_t index = 1; index < buffer.size(); index++) {
            if (buffer[index - 1] == '\r' and buffer[index] == '\n') {
                messageLen = index + 1;
                break;
            }
        }

        return messageLen;
    }

    /// @brief Returns first network message.
    std::string popFirstNetworkMessage() {
        int messageLen = networkMessageLen();
        std::string message = buffer.substr(0, messageLen);
        buffer = buffer.substr(messageLen);

        return message;
    }

    /// @brief Returns length of a message ending with \ n.
    int userMessageLen() {
        if (buffer.empty()) {
            return 0;
        }

        int messageLen = 0;
        for (size_t index = 0; index < buffer.size(); index++) {
            if (buffer[index] == '\n') {
                messageLen = index + 1;
                break;
            }
        }

        return messageLen;
    }

    /// @brief Returns first user message.
    std::string popFirstUserMessage() {
        int messageLen = userMessageLen();
        std::string message = buffer.substr(0, messageLen);
        buffer = buffer.substr(messageLen);

        return message;
    }
};

struct WriteBuffer {
    std::deque<std::string> messages;
    std::string currentMessage;

    /// @brief Appends message to buffer.
    void appendMessage(std::string message) {
        if (messages.empty()) {
            currentMessage = message;
        }

        messages.push_back(message);
    }

    /// @brief Returns current message's 2 letter prefix.
    std::string getCurrentMessage() {
        return currentMessage;
    }

    /// @brief Returns true if there is message and false otherwise.
    bool hasMessage() {
        return not messages.empty();
    }

    /// @brief Returns first message.
    std::string getFirstMessage() {
        if (messages.empty()) {
            return "";
        }

        return messages.front();
    }

    /// @brief Returns true if wrote whole message and handles operations of shortening/popping.
    bool wroteWholeMessage(int bytesWrote) {
        if (bytesWrote == (int)messages.front().size()) {
            messages.pop_front();
            currentMessage.clear();

            if (messages.empty()) {
                return true;
            }

            currentMessage = messages.front();

            return true;
        }

        std::string firstMessage = messages.front();
        std::string partialMessage = firstMessage.substr(bytesWrote, firstMessage.size());

        messages.pop_front();
        messages.push_front(partialMessage);
        return false;
    }
};

struct Card {
    CARD_COLOR cardColor;
    int cardValue;

    bool operator==(const Card &other) const {
        return (cardColor == other.cardColor) and (cardValue == other.cardValue);
    }

    bool operator!=(const Card &other) const {
        return not(*this == other);
    }

    /// @brief Returns card in a string format.
    std::string toStr() {
        std::string cardStr;
        if (cardValue > 10) {
            switch (cardValue) {
            case 11:
                cardStr = 'J';
                break;
            case 12:
                cardStr = 'Q';
                break;
            case 13:
                cardStr = 'K';
                break;
            case 14:
                cardStr = 'A';
                break;
            default:
                cardStr = ' ';
            }
        } else {
            cardStr = std::to_string(cardValue);
        }

        switch (cardColor) {
        case CARD_COLOR::C:
            cardStr += 'C';
            break;
        case CARD_COLOR::D:
            cardStr += 'D';
            break;
        case CARD_COLOR::H:
            cardStr += 'H';
            break;
        case CARD_COLOR::S:
            cardStr += 'S';
            break;
        default:
            cardStr += ' ';
        }

        return cardStr;
    }
};

/// FUNCTIONS ///

bool canBeIam(const std::string &message);

bool canBeBusy(const std::string &message);

bool canBeDeal(const std::string &message);

bool canBeTrick(const std::string &message);

bool canBeWrong(const std::string &message);

bool canBeTaken(const std::string &message);

bool canBeScore(const std::string &message);

bool canBeTotal(const std::string &message);

char tablePlaceToChar(int c);

TABLE_PLACE charToTablePlace(const char &c);

HAND_TYPE charToHandType(const char &c);

bool setCardFromStr(Card &card, const std::string &str);

std::string getCardsStr(std::vector<Card> &cards);

std::pair<bool, std::vector<Card>> parseCardsVector(const std::string &message, size_t start,
                                                    size_t end);

void display(const std::string &sender, const std::string &receiver, const std::string &message);

ssize_t sendMessage(int socketFd, const void *vptr, size_t n);

bool prefixEqual(const std::string &message, std::string expected);

std::pair<int, int> getTrickNumber(std::string &message, int numberStart);

uint16_t readPort(char const *string);

std::string getIpv4AndPortAddress(struct sockaddr_in addressIpv4);

std::string getIpv6AndPortAddress(struct sockaddr_in6 addressIpv6);

void setServerAddressIpv4(uint16_t port, struct addrinfo *address_result,
                          struct sockaddr_in *serverAddress);

void setServerAddressIpv6(uint16_t port, struct addrinfo *address_result,
                          struct sockaddr_in6 *serverAddress);

#endif // KIERKI_COMMON_H
