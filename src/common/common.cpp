#include "common/common.h"

/// @brief Returns true if message prefix is IAM.
bool canBeIam(const std::string &message) {
    return prefixEqual(message, Messages::IAM);
}

/// @brief Returns true if message prefix is BUSY.
bool canBeBusy(const std::string &message) {
    return prefixEqual(message, Messages::BUSY);
}

/// @brief Returns true if message prefix is DEAL.
bool canBeDeal(const std::string &message) {
    return prefixEqual(message, Messages::DEAL);
}

/// @brief Returns true if message prefix is TRICK.
bool canBeTrick(const std::string &message) {
    return prefixEqual(message, Messages::TRICK);
}

/// @brief Returns true if message prefix is WRONG.
bool canBeWrong(const std::string &message) {
    return prefixEqual(message, Messages::WRONG);
}

/// @brief Returns true if message prefix is TAKEN.
bool canBeTaken(const std::string &message) {
    return prefixEqual(message, Messages::TAKEN);
}

/// @brief Returns true if message prefix is SCORE.
bool canBeScore(const std::string &message) {
    return prefixEqual(message, Messages::SCORE);
}

/// @brief Returns true if message prefix is TOTAL.
bool canBeTotal(const std::string &message) {
    return prefixEqual(message, Messages::TOTAL);
}

/// @brief Returns proper card color or undefined.
static CARD_COLOR charToCardColor(char c) {
    switch (c) {
    case 'C':
        return CARD_COLOR::C;
    case 'D':
        return CARD_COLOR::D;
    case 'H':
        return CARD_COLOR::H;
    case 'S':
        return CARD_COLOR::S;
    default:
        return CARD_COLOR::UNDEFINED;
    }
}

/// @brief returns special card value or undefined.
static CARD_VALUE charToCardValue(char c) {
    switch (c) {
    case 'J':
        return CARD_VALUE::J;
    case 'Q':
        return CARD_VALUE::Q;
    case 'K':
        return CARD_VALUE::K;
    case 'A':
        return CARD_VALUE::A;
    default:
        return CARD_VALUE::UNDEFINED;
    }
}

/// @brief Returns true if character is card end.
static bool isCardEnd(char c) {
    return c == 'C' or c == 'D' or c == 'H' or c == 'S';
}

/// @brief converts table place int to char.
char tablePlaceToChar(const int c) {
    switch (c) {
    case 0:
        return 'N';
    case 1:
        return 'E';
    case 2:
        return 'S';
    case 3:
        return 'W';
    default:
        return ' ';
    }
}

/// @brief converts char to table place.
TABLE_PLACE charToTablePlace(const char &c) {
    switch (c) {
    case 'N':
        return TABLE_PLACE::N;
    case 'E':
        return TABLE_PLACE::E;
    case 'S':
        return TABLE_PLACE::S;
    case 'W':
        return TABLE_PLACE::W;
    default:
        return TABLE_PLACE::UNDEFINED;
    }
}

/// @brief converts char to hand type.
HAND_TYPE charToHandType(const char &c) {
    if ('1' <= c and c <= '7')
        return static_cast<HAND_TYPE>(c - '0');
    return HAND_TYPE::UNDEFINED;
}

/// @brief Returns true and sets card from string and returns false otherwise.
bool setCardFromStr(Card &card, const std::string &str) {
    size_t sz = str.size();

    // First we need to set the color of the card.
    card.cardColor = charToCardColor(str[sz - 1]);
    if (card.cardColor == CARD_COLOR::UNDEFINED) {
        return false;
    }

    if (sz == 3 and str[0] == '1' and str[1] == '0') { // { card value == 10 }
        card.cardValue = 10;
        return true;
    }

    if (sz != 2) {
        return false;
    } // { sz == 2 }

    if (str[0] >= '2' and str[0] <= '9') {
        card.cardValue = str[0] - '0';
        return true;
    }

    CARD_VALUE cardValue = charToCardValue(str[0]);
    if (cardValue == CARD_VALUE::UNDEFINED) {
        return false;
    }

    card.cardValue = static_cast<int>(cardValue);

    return true;
}

/// @brief Returns cards string from vector.
std::string getCardsStr(std::vector<Card> &cards) {
    std::string str = "";
    for (auto &card : cards) {
        str += card.toStr();
    }
    return str;
}

/// @brief Function returns {true, cards vector} if valid message and {false, {}} otherwise.
/// List of cards stats at message[start] and ends at message[end - 1].
std::pair<bool, std::vector<Card>> parseCardsVector(const std::string &message, const size_t start,
                                                    const size_t end) {
    std::vector<Card> cards;
    size_t cardStart = start;

    for (size_t i = start; i < end; i++) {
        if (not isCardEnd(message[i])) {
            continue;
        }

        std::string cardStr = message.substr(cardStart, (i - cardStart + 1));
        Card currentCard = Card();
        if (not setCardFromStr(currentCard, cardStr)) {
            return {false, {}};
        }

        // We append the card to client's hand.
        cards.emplace_back(currentCard);
        cardStart = i + 1;
    }

    // Last card should end at message[end - 1].
    if (cardStart != end) {
        return {false, {}};
    }

    return {true, cards};
}

/// @brief Returns current date time to display.
static std::string currentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

/// @brief Displays message from sender to receiver.
void display(const std::string &sender, const std::string &receiver, const std::string &message) {
    std::cout << "[" << sender << "," << receiver << "," << currentDateTime() << "] " << message
              << std::flush;
}

/// @brief Function is a wrapper for write.
ssize_t sendMessage(int socketFd, const void *vptr, size_t n) {
    return write(socketFd, vptr, n);
}

/// @brief Returns true if prefix of message is equal to expected and false otherwise.
bool prefixEqual(const std::string &message, std::string expected) {
    return message.size() >= expected.size() and message.substr(0, expected.size()) == expected;
}

/// @brief Returns true if character is one of the special card values.
static bool isSpecialCardValue(const char c) {
    return c == 'J' or c == 'Q' or c == 'K' or c == 'A';
}

/// @brief Returns {trick number, card start index} if valid and {-1, -1} otherwise.
std::pair<int, int> getTrickNumber(std::string &message, int numberStart) {
    int numberEnd = numberStart, numberLen = 0, trickEnd;

    while (message[numberEnd] >= '0' and message[numberEnd] <= '9')
        numberEnd++, numberLen++;

    if (numberLen > 4 or numberLen < 1) {
        return {-1, -1};
    }

    if (numberLen == 4) { // { 10 <= trick <= 13 and card value = 10 }
        trickEnd = numberStart + 1;
    } else if (numberLen == 3) { // { card value == 10 ? |card value| = 2 : |card value| = 1 }
        trickEnd = numberStart + (message[numberEnd - 1] != '0');
    } // { numberLen <= 2 }
    else if (isSpecialCardValue(message[numberEnd])) {
        trickEnd = numberStart + (numberLen == 2);
    } else {
        trickEnd = numberStart + (numberLen == 2 and message[numberEnd] == '\r');
    }

    int trickLen = trickEnd - numberStart + 1;
    int trickNum = stoi(message.substr(numberStart, trickEnd - numberStart + 1));

    if (trickLen == 1 and trickNum >= 1 and trickNum <= 9) {
        return {trickNum, trickEnd + 1};
    } else if (trickLen == 2 and trickNum >= 10 and trickNum <= 13) {
        return {trickNum, trickEnd + 1};
    }

    return {-1, -1};
}

/// @brief Function returns port.
uint16_t readPort(char const *string) {
    char *endptr;
    errno = 0;
    unsigned long port = strtoul(string, &endptr, 10);
    if (errno != 0 or *endptr != 0 or port > UINT16_MAX) {
        fatal("%s is not a valid port number", string);
    }
    return (uint16_t)port;
}

/// @brief Function returns ipv4 string to log.
std::string getIpv4AndPortAddress(struct sockaddr_in addressIpv4) {
    char addressIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addressIpv4.sin_addr, addressIp, INET_ADDRSTRLEN);
    uint16_t port = ntohs(addressIpv4.sin_port);

    std::stringstream ss;
    ss << addressIp << ":" << port;
    return ss.str();
}

/// @brief Function returns ipv6 string to log.
std::string getIpv6AndPortAddress(struct sockaddr_in6 addressIpv6) {
    char addressIp[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &addressIpv6.sin6_addr, addressIp, INET6_ADDRSTRLEN);
    uint16_t port = ntohs(addressIpv6.sin6_port);

    std::stringstream ss;
    ss << addressIp << ":" << port;
    return ss.str();
}

/// @brief Function sets server address ipv4.
void setServerAddressIpv4(uint16_t port, struct addrinfo *address_result,
                          struct sockaddr_in *serverAddress) {
    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin_family = AF_INET; // IPv4
    serverAddress->sin_addr.s_addr =
        ((struct sockaddr_in *)(address_result->ai_addr))->sin_addr.s_addr;
    serverAddress->sin_port = htons(port);

    freeaddrinfo(address_result);
}

/// @brief Function sets server address ipv6.
void setServerAddressIpv6(uint16_t port, struct addrinfo *address_result,
                          struct sockaddr_in6 *serverAddress) {
    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin6_family = AF_INET6; // IPv6
    serverAddress->sin6_addr = ((struct sockaddr_in6 *)(address_result->ai_addr))->sin6_addr;
    serverAddress->sin6_port = htons(port);

    freeaddrinfo(address_result);
}
