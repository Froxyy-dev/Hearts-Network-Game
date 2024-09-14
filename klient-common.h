#ifndef KIERKI_KLIENT_COMMON_H
#define KIERKI_KLIENT_COMMON_H

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

namespace ClientConstants {
const int USER_INDEX = 0;
const int SERVER_INDEX = 1;
const int CLIENT_CONNECTIONS = 2;
const int BUFFER_SIZE = 1024;
const int CLIENT_TIMEOUT = -1;
const std::string USER_CARDS = "cards\n";
const std::string USER_TRICKS = "tricks\n";
const int GAME_FINISHED = 1;
const int GAME_NOT_FINISHED = 0;
} // namespace ClientConstants

/// STRUCTS ///

struct ClientArguments {
    char *host;
    char *port;
    int aiFamily;
    TABLE_PLACE tablePlace;
    bool isAutomatic;

    ClientArguments() {
        host = nullptr;
        port = nullptr;
        aiFamily = AF_UNSPEC;
        isAutomatic = false;
        tablePlace = TABLE_PLACE::UNDEFINED;
    }

    /// @brief Sets client table place from character.
    void setTablePlace(char c) {
        switch (c) {
        case 'N':
            tablePlace = TABLE_PLACE::N;
            break;
        case 'E':
            tablePlace = TABLE_PLACE::E;
            break;
        case 'S':
            tablePlace = TABLE_PLACE::S;
            break;
        case 'W':
            tablePlace = TABLE_PLACE::W;
            break;
        default:
            tablePlace = TABLE_PLACE::UNDEFINED;
            break;
        }
    }
};

struct ClientHand {
    HAND_TYPE handType;
    TABLE_PLACE previousTrickTaker;
    TABLE_PLACE clientPlace;
    int trickNumber = 1;

    bool firstMessage = true;
    bool firstDeal = false; // After first deal we might get taken.
    bool previousDeal = false;
    bool previousTrick = false;
    bool previousTaken = false;
    bool previousScore = false;
    bool previousTotal = false;
    bool sentTrick = false;
    int countResults = 0;

    std::vector<Card> clientCards;

    /// @brief Deletes card from vector in O(1) complexity.
    void deleteCard(int index) {
        // Swap with last element.
        std::swap(clientCards[index], clientCards.back());

        // Delete last element.
        clientCards.pop_back();
    }

    /// @brief We can receive busy if it is first message from server.
    bool waitingForBusy() {
        return firstMessage;
    }

    /// @brief We can receive deal if it is first message or we got both results.
    bool waitingForDeal() {
        return firstMessage or (countResults == 2);
    }

    /// @brief We can receive trick if previous was deal, taken or trick.
    bool waitingForTrick() {
        return previousDeal or previousTaken or previousTrick;
    }

    /// @brief We can receive taken after first deal if client disconnected or after trick.
    bool waitingForTaken() {
        return firstDeal or previousTrick;
    }

    /// @brief We can receive wrong after trick.
    bool waitingForWrong() {
        return previousTrick;
    }

    /// @brief We can receive score after taken or after total.
    bool waitingForScore() {
        return (countResults == 0 and previousTaken) or (countResults == 1 and previousTotal);
    }

    /// @brief We can receive total after taken or after score.
    bool waitingForTotal() {
        return (countResults == 0 and previousTaken) or (countResults == 1 and previousScore);
    }
};

/// FUNCTIONS ///

void displayCardsVector(std::vector<Card> &cards, bool endWithDot);

std::string cardToTrick(Card &card, ClientHand clientHand);

std::string strTrickClient(std::vector<Card> &currentCards, ClientHand clientHand);

#endif // KIERKI_KLIENT_COMMON_H
