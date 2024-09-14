#include "serwer-common.h"

/// @brief Function adds points to client.
static void addPoints(ServerHand &hand, TABLE_PLACE client, int points) {
    hand.playerScores[client] += points;
}

/// @brief Function handles bandit (hand type 7).
static void handleBandit(ServerHand &hand, TABLE_PLACE client,
                         const std::vector<Card> &takenCards) {
    addPoints(hand, client, 1);

    for (const auto card : takenCards) {
        if (card.cardColor == CARD_COLOR::H) {
            addPoints(hand, client, 1);
        }
        if (card.cardValue == static_cast<int>(CARD_VALUE::Q)) {
            addPoints(hand, client, 5);
        }
        if (card.cardValue == static_cast<int>(CARD_VALUE::J) or
            card.cardValue == static_cast<int>(CARD_VALUE::K)) {
            addPoints(hand, client, 2);
        }
        if (card.cardValue == static_cast<int>(CARD_VALUE::K) and card.cardColor == CARD_COLOR::H) {
            addPoints(hand, client, 18);
        }
    }

    if (hand.currentTrick == 7 or hand.currentTrick == Constants::TRICK_NUMBER) {
        addPoints(hand, client, 10);
    }
}

/// @brief Function adjust points based on hand type.
static void adjustPoints(ServerHand &hand, TABLE_PLACE client,
                         const std::vector<Card> &takenCards) {
    switch (hand.handType) {
    case HAND_TYPE::DEFAULT:
        addPoints(hand, client, 1);
        break;

    case HAND_TYPE::HEART:
        for (const auto card : takenCards) {
            if (card.cardColor == CARD_COLOR::H)
                addPoints(hand, client, 1);
        }
        break;

    case HAND_TYPE::QUEEN:
        for (const auto card : takenCards) {
            if (card.cardValue == static_cast<int>(CARD_VALUE::Q))
                addPoints(hand, client, 5);
        }
        break;

    case HAND_TYPE::GUYS:
        for (const auto card : takenCards) {
            if (card.cardValue == static_cast<int>(CARD_VALUE::J) or
                card.cardValue == static_cast<int>(CARD_VALUE::K))
                addPoints(hand, client, 2);
        }
        break;

    case HAND_TYPE::HEART_KING:
        for (const auto card : takenCards) {
            if (card.cardValue == static_cast<int>(CARD_VALUE::K) and
                card.cardColor == CARD_COLOR::H)
                addPoints(hand, client, 18);
        }
        break;

    case HAND_TYPE::SEVEN_N_LAST:
        if (hand.currentTrick == 7 or hand.currentTrick == Constants::TRICK_NUMBER)
            addPoints(hand, client, 10);
        break;

    case HAND_TYPE::BANDIT:
        handleBandit(hand, client, takenCards);
        break;

    default:
        break;
    }
}

/// @brief Function returns client with highest card and adjust his points.
static char takesTrick(ServerHand &hand) {
    int currentIndex = static_cast<int>(hand.previousTrickTaker);
    char highestClient = tablePlaceToChar(currentIndex);

    Card highestCard = hand.currentlyPlacedCards[0];

    for (int i = 1; i < Constants::PLAYERS_NUMBER; i++) {
        currentIndex = (currentIndex + 1) % Constants::PLAYERS_NUMBER;

        if (hand.currentlyPlacedCards[i].cardColor == highestCard.cardColor and
            hand.currentlyPlacedCards[i].cardValue > highestCard.cardValue) {

            highestClient = tablePlaceToChar(currentIndex);
            hand.previousTrickTaker = static_cast<TABLE_PLACE>(currentIndex);

            highestCard = hand.currentlyPlacedCards[i];
        }
    }

    adjustPoints(hand, charToTablePlace(highestClient), hand.currentlyPlacedCards);

    return highestClient;
}

/// @brief Function returns taken string and sets previous trick taker.
std::string getTakenStr(ServerHand &hand) {
    std::string message = Messages::TAKEN;
    message += std::to_string(hand.currentTrick);
    message += getCardsStr(hand.currentlyPlacedCards);
    message.push_back(takesTrick(hand));
    message += Messages::END_OF_MESSAGE;

    return message;
}
