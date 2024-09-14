#include "klient-common.h"

/// @brief Displays cards from vector.
void displayCardsVector(std::vector<Card> &cards, bool endWithDot) {
    for (int i = 0; i < (int)cards.size(); i++) {
        std::cout << cards[i].toStr();

        if (i != (int)cards.size() - 1)
            std::cout << ", ";
        else if (endWithDot)
            std::cout << ".";
    }
    std::cout << std::endl;
}

/// @brief Returns trick message to send.
std::string cardToTrick(Card &card, ClientHand clientHand) {
    std::string cardStr = card.toStr();

    std::string message = Messages::TRICK;
    message += std::to_string(clientHand.trickNumber) + cardStr;
    message += Messages::END_OF_MESSAGE;

    return message;
}

/// @brief Automatic selecting card for trick.
std::string strTrickClient(std::vector<Card> &currentCards, ClientHand clientHand) {
    int cardsNum = (int)clientHand.clientCards.size() - 1;
    Card selectedCard = clientHand.clientCards[cardsNum];

    if (not currentCards.empty()) {
        Card firstCard = currentCards[0];

        for (int i = 0; i < cardsNum; i++) {
            if (firstCard.cardColor == clientHand.clientCards[i].cardColor) {
                selectedCard = clientHand.clientCards[i];
                break;
            }
        }
    }

    return cardToTrick(selectedCard, clientHand);
}