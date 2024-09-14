#include "server/serwer-communicator.h"

/// Every message that is parsed here ends with "\r\n". ///

/// @brief Returns TABLE_PLACE from IAM message or UNDEFINED if error occurred.
TABLE_PLACE parseIam(const std::string &message) {
    // message ? IAM.\r\n
    if (message.size() != 6 or not canBeIam(message)) {
        return TABLE_PLACE::UNDEFINED;
    }

    return charToTablePlace(message[3]);
}

/// @brief Returns busy message.
std::string getBusyMessage(ServerContext &serverContext) {
    std::string message = Messages::BUSY;
    for (int i = 0; i < ServerConstants::ACCEPT_INDEX; i++) {
        if (serverContext.isDescriptorReserved(i)) {
            message += tablePlaceToChar(i);
        }
    }

    message += Messages::END_OF_MESSAGE;

    return message;
}

/// @brief Appends deal and (if client disconnected) taken messages to given write buffer.
void setDealTakenMessage(TABLE_PLACE tablePlace, ServerStatus &serverStatus,
                         ServerContext &serverContext) {
    int index = static_cast<int>(tablePlace);
    std::string dealMessage = serverStatus.getCurrentHand().dealStrAtPlace[tablePlace];

    serverContext.appendMessageToWriteAt(index, dealMessage);

    for (const auto &takenStr : serverStatus.getPreviousTakenList()) {
        serverContext.appendMessageToWriteAt(index, takenStr);
    }
}

/// @brief Returns trick message.
std::string getTrickMessage(ServerStatus &serverStatus) {
    ServerHand hand = serverStatus.getCurrentHand();

    std::string message = Messages::TRICK;
    message += std::to_string(hand.currentTrick);
    message += getCardsStr(hand.currentlyPlacedCards);
    message += Messages::END_OF_MESSAGE;

    return message;
}

/// @brief Returns true if trick message can be parsed.
bool canTrickBeParsed(std::string message) {
    if (message.size() < 9 or not canBeTrick(message)) {
        return false;
    }

    auto [trickNum, cardsStart] = getTrickNumber(message, 5);
    if (trickNum == -1) {
        return false;
    }

    auto [isCorrect, placedCards] = parseCardsVector(message, cardsStart, message.size() - 2);
    if (not isCorrect or placedCards.size() != 1) {
        return false;
    }

    return true;
}

/// @brief Returns true and deletes card if client send correct TRICK message and false otherwise.
bool parseTrickServer(std::string message, ServerStatus &serverStatus, TABLE_PLACE currentPlayer) {
    // message ? TRICK..\r\n
    if (message.size() < 9 or not canBeTrick(message)) {
        return false;
    }

    auto [trickNum, cardsStart] = getTrickNumber(message, 5);
    if (trickNum != serverStatus.getCurrentTrick()) {
        return false;
    }

    auto [isCorrect, placedCards] = parseCardsVector(message, cardsStart, message.size() - 2);
    if (not isCorrect or placedCards.size() != 1) {
        return false;
    }

    return serverStatus.playerPlacesCard(currentPlayer, placedCards.back());
}

/// @brief Returns wrong message.
std::string getWrongMessage(ServerStatus &serverStatus) {
    std::string message = Messages::WRONG;
    message += std::to_string(serverStatus.getCurrentHand().currentTrick);
    message += Messages::END_OF_MESSAGE;

    return message;
}

/// @brief Returns results string specified in which.
std::string getResultsMessage(ServerStatus &serverStatus, const std::string &which) {
    std::string message = which;

    if (which == "SCORE") {
        for (auto &[place, score] : serverStatus.getCurrentHand().playerScores) {
            message += tablePlaceToChar(static_cast<int>(place));
            message += std::to_string(score);
        }
    } else {
        for (auto &[place, score] : serverStatus.playerTotalScores) {
            message += tablePlaceToChar(static_cast<int>(place));
            message += std::to_string(score);
        }
    }

    message += Messages::END_OF_MESSAGE;
    return message;
}
