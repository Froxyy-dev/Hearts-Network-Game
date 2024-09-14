#include "client/klient-communicator.h"

/// FUNCTIONS FOR DISPLAYING MESSAGES FROM SERVER. ///

/// @brief Function for displaying list of busy places.
static void displayBusyPlaces(std::vector<bool> busyPlaces, int countBusy) {
    std::cout << "Place busy, list of busy places received: ";
    for (int i = 0; i < Constants::PLAYERS_NUMBER; i++) {
        if (not busyPlaces[i]) {
            continue;
        }

        std::cout << tablePlaceToChar(i);

        if (--countBusy != 0)
            std::cout << ", ";
        else
            std::cout << ".";
    }
    std::cout << std::endl;
}

/// @brief Function for displaying deal information.
static void displayDealInformation(ClientHand clientHand) {
    int handType = static_cast<int>(clientHand.handType);
    char startingClient = tablePlaceToChar(static_cast<int>(clientHand.previousTrickTaker));

    std::cout << "New deal " << handType << ": staring place " << startingClient
              << ", your cards: ";
    displayCardsVector(clientHand.clientCards, true);
}

/// @brief Function for displaying trick information.
static void displayTrickInformation(int trickNum, std::vector<Card> placedCards,
                                    ClientHand clientHand) {
    std::cout << "Trick: (" << trickNum << ") ";
    displayCardsVector(placedCards, false);

    std::cout << "Available: ";
    displayCardsVector(clientHand.clientCards, false);
    std::cout << "Waiting for card. " << Constants::AVAILABLE_COMMANDS << std::flush;
}

/// @brief Function for displaying taken information.
static void displayTakenInformation(int trickNum, std::vector<Card> placedCards,
                                    TABLE_PLACE takesTrick) {
    char takesTrickChar = tablePlaceToChar(static_cast<int>(takesTrick));

    std::cout << "A trick: " << trickNum << " is taken by " << takesTrickChar << ", cards ";
    displayCardsVector(placedCards, true);
}

/// @brief Function for displaying results information.
static void displayResultsInformation(std::string expected,
                                      const std::vector<std::pair<char, int>> &scoresList) {
    if (expected == "TOTAL") {
        std::cout << "The total scores are:\n";
    } else {
        std::cout << "The scores are:\n";
    }

    for (auto &[place, score] : scoresList) {
        std::cout << place << " | " << score << std::endl;
    }
}

/// @brief Returns Iam message.
std::string getIamMessage(ClientArguments client_arguments) {
    std::string message = Messages::IAM;
    message += tablePlaceToChar(static_cast<int>(client_arguments.tablePlace));
    message += Messages::END_OF_MESSAGE;

    return message;
}

/// @brief Returns True if message is valid BUSY and false otherwise.
bool parseBusy(const std::string &message, ClientContext &clientContext) {
    // message ? BUSY....\r\n
    if (message.size() < 7 or message.size() > 10 or not canBeBusy(message)) {
        return false;
    }

    std::vector<bool> busyPlaces(Constants::PLAYERS_NUMBER, false);
    int countBusy = 0;

    for (int i = 4; i < (int)message.size() - 2; i++) {
        int tablePlace = static_cast<int>(charToTablePlace(message[i]));

        if (tablePlace == static_cast<int>(TABLE_PLACE::UNDEFINED) or busyPlaces[tablePlace]) {
            return false;
        }

        countBusy++;
        busyPlaces[tablePlace] = true;
    }

    if (not clientContext.isClientAutomatic()) {
        displayBusyPlaces(busyPlaces, countBusy);
    }

    return true;
}

/// @brief Returns True if message is valid deal and false otherwise.
bool parseDeal(std::string message, ClientContext &clientContext) {
    if (message.size() < 9 or not canBeDeal(message)) {
        return false;
    }

    HAND_TYPE handType = charToHandType(message[4]);
    TABLE_PLACE previousTrickTaker = charToTablePlace(message[5]);
    if (handType == HAND_TYPE::UNDEFINED or previousTrickTaker == TABLE_PLACE::UNDEFINED) {
        return false;
    }

    clientContext.setHandType(handType);
    clientContext.setPreviousTrickTaker(previousTrickTaker);

    auto [isCorrect, playerCards] = parseCardsVector(message, 6, message.size() - 2);
    if (not isCorrect or playerCards.size() != Constants::CARDS_NUMBER) {
        return false;
    }
    clientContext.setPlayerCards(playerCards);

    if (not clientContext.isClientAutomatic()) {
        displayDealInformation(clientContext.getClientHand());
    }

    return true;
}

/// @brief Returns {True, list of placed cards} if message is valid TRICK or {False, {}} otherwise.
std::pair<bool, std::vector<Card>> parseTrickClient(std::string message,
                                                    ClientContext &clientContext) {
    // message ? TRICK....\r\n
    if (message.size() < 7 or not canBeTrick(message)) {
        return {false, {}};
    }

    auto [trickNum, cardsStart] = getTrickNumber(message, 5);
    if (trickNum == -1) {
        return {false, {}};
    }

    auto [isCorrect, placedCards] = parseCardsVector(message, cardsStart, message.size() - 2);
    if (not isCorrect) {
        return {false, {}};
    }

    if (not clientContext.isClientAutomatic()) {
        displayTrickInformation(trickNum, placedCards, clientContext.getClientHand());
    }

    return {true, placedCards};
}

/// @brief Returns client index in current trick.
static int getClientIndex(int clientPlace, int previousTrickTaker) {
    return (clientPlace - previousTrickTaker + Constants::PLAYERS_NUMBER) %
           Constants::PLAYERS_NUMBER;
}

/// @brief Returns a vector of placed cards if message is valid taken and empty vector otherwise.
std::vector<Card> parseTaken(std::string message, ClientContext &clientContext) {
    // message ? TAKEN....\r\n
    if (message.size() < 7 or not canBeTaken(message)) {
        return {};
    }

    auto [trickNum, cardsStart] = getTrickNumber(message, 5);
    if (trickNum == Constants::ERROR_CODE) {
        return {};
    }

    auto [isCorrect, placedCards] = parseCardsVector(message, cardsStart, message.size() - 3);
    if (not isCorrect or placedCards.size() != Constants::PLAYERS_NUMBER) {
        return {};
    }

    TABLE_PLACE takesTrick = charToTablePlace(message[message.size() - 3]);
    if (takesTrick == TABLE_PLACE::UNDEFINED) {
        return {};
    }

    ClientHand clientHand = clientContext.getClientHand();
    int clientPlaceInt = static_cast<int>(clientHand.clientPlace);
    int previousTrickTakerInt = static_cast<int>(clientHand.previousTrickTaker);

    int clientIndex = getClientIndex(clientPlaceInt, previousTrickTakerInt);

    int cardIndex = -1;
    for (int i = 0; i < (int)clientHand.clientCards.size(); i++) {
        if (placedCards[clientIndex] == clientHand.clientCards[i]) {
            cardIndex = i;
        }
    }
    if (cardIndex == -1) {
        return {};
    }

    clientContext.afterTaken(takesTrick, cardIndex);

    if (not clientContext.isClientAutomatic()) {
        displayTakenInformation(trickNum, placedCards, takesTrick);
    }

    return placedCards;
}

/// @brief Returns True if message is a valid wrong and false otherwise.
bool parseWrong(std::string message, ClientContext &clientContext) {
    // message ? WRONG....\r\n
    if (message.size() < 7 or not canBeWrong(message)) {
        return false;
    }

    int numberStart = 5, numberEnd = 5, numberLen = 0;
    while (message[numberEnd] >= '0' and message[numberEnd] <= '9') {
        numberEnd++, numberLen++;
    }
    if (numberLen == 0 or numberLen > 2) {
        return false;
    }

    int trickNum = stoi(message.substr(numberStart, numberLen));
    if (numberLen == 1 and trickNum == 0) {
        return false;
    }
    if (numberLen == 2 and (trickNum < 10 or trickNum > Constants::TRICK_NUMBER)) {
        return false;
    }

    if (not clientContext.isClientAutomatic()) {
        std::cout << "Wrong message received in trick " << trickNum << "." << std::endl;
    }

    return true;
}

/// @brief Returns true if message is a valid result and false otherwise.
bool parseResults(std::string message, const std::string &expected, ClientContext &clientContext) {
    // message ? [expected]....\r\n
    if (message.size() < 7 or not prefixEqual(message, expected)) {
        return false;
    }

    std::vector<std::pair<char, int>> scoresList;

    int clientPlace = 5, numberEnd = 6, numberLen;
    for (int i = 0; i < 4 and clientPlace < (int)message.size() - 2; i++) {
        if (charToTablePlace(message[clientPlace]) == TABLE_PLACE::UNDEFINED) {
            return false;
        }

        numberLen = 0;
        while (numberEnd < (int)message.size() and message[numberEnd] >= '0' and
               message[numberEnd] <= '9') {
            numberEnd++, numberLen++;
        }
        if (numberLen == 0) {
            return false;
        }

        int clientScore = stoi(message.substr(clientPlace + 1, numberLen));

        scoresList.emplace_back(message[clientPlace], clientScore);
        clientPlace = numberEnd++;
    }

    if (clientPlace != (int)message.size() - 2) {
        return false;
    }

    if (not clientContext.isClientAutomatic()) {
        displayResultsInformation(expected, scoresList);
    }

    return true;
}
