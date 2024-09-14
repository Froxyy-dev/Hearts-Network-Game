#ifndef KIERKI_SERWER_COMMON_H
#define KIERKI_SERWER_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <sys/poll.h>
#include <sys/types.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <netdb.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <sys/poll.h>

#include "common.h"

namespace ServerConstants {
const int ACCEPT_INDEX = 4;
const int CONNECTIONS = 512;
const int DEFAULT_TIMEOUT = 5;
const int DEFAULT_PORT = 0;
const int QUEUE_LENGTH = 5;
const int BUFFER_SIZE = 1024;
const std::string GAME_FULL_MESSAGE = "BUSYNESW\r\n";
} // namespace ServerConstants

struct ServerArguments {
    char *portStr;
    char *fileStr;
    char *timeoutStr;

    int timeout;
    uint16_t port;

    ServerArguments() {
        portStr = nullptr;
        fileStr = nullptr;
        timeoutStr = nullptr;
        timeout = ServerConstants::DEFAULT_TIMEOUT;
        port = ServerConstants::DEFAULT_PORT;
    }
};

struct ServerHand {
    HAND_TYPE handType;
    TABLE_PLACE previousTrickTaker;
    TABLE_PLACE currentClient;

    int currentTrick = 1;
    std::vector<Card> currentlyPlacedCards;

    std::map<TABLE_PLACE, std::string> dealStrAtPlace;
    std::vector<std::string> previousTaken;

    std::map<TABLE_PLACE, std::vector<Card>> playerCards;
    std::map<TABLE_PLACE, uint64_t> playerScores;

    void deleteCard(int index) {
        // Swap with last element.
        std::swap(playerCards[currentClient][index], playerCards[currentClient].back());

        // Delete last element.
        playerCards[currentClient].pop_back();
    }
};

struct ServerStatus {
    int activePlayers = 0;
    int currentHand = 0;

    std::vector<ServerHand> hands;
    std::map<TABLE_PLACE, uint64_t> playerTotalScores;
    std::map<TABLE_PLACE, bool> dealSend;
    std::map<TABLE_PLACE, bool> alreadyLeft;

    bool gameStarted = false;
    bool gameEnded = false;

    /// @brief Returns true if everyone left and server can finish.
    bool hasEveryoneLeft() {
        return alreadyLeft.size() == Constants::PLAYERS_NUMBER;
    }

    /// @brief Returns current hand.
    ServerHand getCurrentHand() {
        return hands[currentHand];
    }

    /// @brief Returns current trick.
    int getCurrentTrick() {
        return hands[currentHand].currentTrick;
    }

    /// @brief Returns set of player's cards.
    std::vector<Card> getPlayerCards(TABLE_PLACE player) {
        return hands[currentHand].playerCards[player];
    }

    /// @brief Returns True if placed card's color is valid and False otherwise.
    bool isValidColor(std::vector<Card> &playerCards, Card card) {
        if (hands[currentHand].currentlyPlacedCards.empty()) {
            return true;
        }

        bool hasMatchingCard = false;
        CARD_COLOR firstPlacedColor = hands[currentHand].currentlyPlacedCards.front().cardColor;

        for (auto playerCard : playerCards) {
            if (playerCard.cardColor != firstPlacedColor) {
                continue;
            }

            hasMatchingCard = true;
            break;
        }

        if (hasMatchingCard and card.cardColor != firstPlacedColor) {
            return false;
        }

        return true;
    }

    /// @brief Returns index in cards if player has card and -1 otherwise.
    static int getCardIndex(std::vector<Card> &playerCards, Card card) {
        for (int i = 0; i < (int)playerCards.size(); i++) {
            if (card == playerCards[i]) {
                return i;
            }
        }

        return -1;
    }

    /// @brief Returns True if placed card is valid and False otherwise.
    bool playerPlacesCard(TABLE_PLACE player, Card card) {
        std::vector<Card> playerCards = getPlayerCards(player);

        int cardIndex;
        if ((cardIndex = getCardIndex(playerCards, card)) == -1) {
            return false;
        }

        if (not isValidColor(playerCards, card)) {
            return false;
        }

        hands[currentHand].deleteCard(cardIndex);
        hands[currentHand].currentlyPlacedCards.emplace_back(card);

        return true;
    }

    /// @brief Returns current player's table place as an integer.
    int getCurrentTablePlace() {
        return static_cast<int>(hands[currentHand].currentClient);
    }

    /// @brief Function sets current table place.
    void setCurrentTablePlace(TABLE_PLACE tablePlace) {
        hands[currentHand].currentClient = tablePlace;
    }

    /// @brief Function returns previous trick taker.
    TABLE_PLACE getPreviousTrickTaker() {
        return hands[currentHand].previousTrickTaker;
    }

    /// @brief Function clears cards placed on table.
    void clearCardsFromTable() {
        hands[currentHand].currentlyPlacedCards.clear();
    }

    /// @brief Function returns True if player at given index received deal message.
    bool playerReceivedDeal(int index) {
        return dealSend[static_cast<TABLE_PLACE>(index)];
    }

    /// @brief Function updates player score.
    void updatePlayerTotalScore(int index) {
        auto place = static_cast<TABLE_PLACE>(index);
        playerTotalScores[place] += hands[currentHand].playerScores[place];
    }

    /// @brief Function updates if deal was sent to player at index.
    void setDealSentAt(int index, bool value) {
        dealSend[static_cast<TABLE_PLACE>(index)] = value;
    }

    /// @brief Returns previous taken list.
    std::vector<std::string> getPreviousTakenList() {
        return hands[currentHand].previousTaken;
    }

    /// @brief Returns true if game is active.
    bool isGameActive() {
        return activePlayers == Constants::PLAYERS_NUMBER;
    }

    /// @brief We want to include players in poll when:
    /// - game is active,
    /// - game has not been started,
    /// - game has already ended.
    bool pollIncludesPlayers() {
        return (activePlayers == Constants::PLAYERS_NUMBER) or (not gameStarted) or gameEnded;
    }

    /// @brief Increases current trick number.
    void finishTrick() {
        hands[currentHand].currentTrick++;
    }

    /// @brief Returns true if current hand has ended.
    bool hasHandEnded() {
        return hands[currentHand].currentTrick == Constants::TRICK_NUMBER + 1;
    }

    /// @brief Increases current hand and sets game ended flag.
    void finishHand() {
        currentHand++;
        if (currentHand == (int)hands.size()) {
            gameEnded = true;
        }
    }
};

std::string getTakenStr(ServerHand &hand);

#endif // KIERKI_SERWER_COMMON_H
