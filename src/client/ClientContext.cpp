#include "client/ClientContext.h"

void ClientContext::createContext(const std::string &_clientAddressStr,
                                  const std::string &_serverAddressStr, int _socketFd,
                                  bool _isAutomatic, TABLE_PLACE _tablePlace) {
    this->clientAddressStr = _clientAddressStr;
    this->serverAddressStr = _serverAddressStr;
    this->socketFd = _socketFd;
    this->isAutomatic = _isAutomatic;
    this->clientHand.clientPlace = _tablePlace;

    userBuffer = ReadBuffer();
    serverBuffer = ReadBuffer();
    writeBuffer = WriteBuffer();
    initializePollStructures();
}

bool ClientContext::isGameFinished() {
    return clientHand.countResults == 2;
}

void ClientContext::initializePollStructures() {
    pollDescriptors[ClientConstants::USER_INDEX].fd = STDIN_FILENO;
    if (isAutomatic) {
        pollDescriptors[ClientConstants::USER_INDEX].events = 0;
    } else {
        pollDescriptors[ClientConstants::USER_INDEX].events = POLLIN;
    }

    pollDescriptors[ClientConstants::SERVER_INDEX].fd = socketFd;
    pollDescriptors[ClientConstants::SERVER_INDEX].events = POLLIN;
}

bool ClientContext::clientCanSendTrick() {
    return not clientHand.sentTrick;
}

void ClientContext::resetRevents() {
    pollDescriptors[ClientConstants::USER_INDEX].revents = 0;
    pollDescriptors[ClientConstants::SERVER_INDEX].revents = 0;
}

bool ClientContext::pollReadFromUser() {
    return not isAutomatic and (pollDescriptors[ClientConstants::USER_INDEX].revents & POLLIN);
}

bool ClientContext::pollReadFromServer() {
    return pollDescriptors[ClientConstants::SERVER_INDEX].revents & POLLIN;
}

bool ClientContext::pollWriteToServer() {
    return pollDescriptors[ClientConstants::SERVER_INDEX].revents & POLLOUT;
}

void ClientContext::displayMessageFromServer(const std::string &message) {
    if (isAutomatic)
        display(serverAddressStr, clientAddressStr, message);
}

void ClientContext::displayMessageFromClient(const std::string &message) {
    if (isAutomatic)
        display(clientAddressStr, serverAddressStr, message);
}

void ClientContext::displayCards() {
    displayCardsVector(clientHand.clientCards, false);
}

void ClientContext::displayTricks() {
    for (auto &trick : takenTricks) {
        displayCardsVector(trick, false);
    }
}

ssize_t ClientContext::sendMessageClient(std::string &message) {
    return sendMessage(socketFd, message.c_str(), message.size());
}

void ClientContext::initiateSending(std::string message) {
    pollDescriptors[ClientConstants::SERVER_INDEX].events |= POLLOUT;

    writeBuffer.appendMessage(message);
}

void ClientContext::setPlayerCards(std::vector<Card> cards) {
    clientHand.clientCards = cards;
}

void ClientContext::setHandType(HAND_TYPE handType) {
    clientHand.handType = handType;
}

void ClientContext::setPreviousTrickTaker(TABLE_PLACE tablePlace) {
    clientHand.previousTrickTaker = tablePlace;
}

void ClientContext::afterTaken(TABLE_PLACE takesTrick, int cardIndex) {
    clientHand.trickNumber++;
    clientHand.previousTrickTaker = takesTrick;
    clientHand.deleteCard(cardIndex);
}

bool ClientContext::tookLastTrick() {
    return clientHand.previousTrickTaker == clientHand.clientPlace;
}

void ClientContext::afterReceivingDeal() {
    // If it was first DEAL, we might get TAKEN.
    if (clientHand.firstMessage)
        clientHand.firstDeal = true;

    clientHand.countResults = 0;
    clientHand.firstMessage = false; // We no longer wait for BUSY/DEAL.
    clientHand.previousDeal = true;  // We wait for TRICK.
}

void ClientContext::afterReceivingTrick() {
    clientHand.previousTaken = false;
    clientHand.previousDeal = false;
    clientHand.firstDeal = false; // We no longer wait for TRICK or TAKEN.
    clientHand.previousTrick = true;
    clientHand.sentTrick = false;
}

void ClientContext::afterReceivingTaken() {
    clientHand.previousDeal = false;
    clientHand.previousTaken = true;
    clientHand.previousTrick = false;
}

void ClientContext::afterReceivingScore() {
    if (clientHand.previousTaken) {
        clientHand.previousTaken = false;
    } else {
        clientHand.previousTotal = false;
    }

    clientHand.previousScore = true;
    clientHand.countResults++;

    if (clientHand.countResults == 2) {
        clientHand.trickNumber = 1;
        clientHand.clientCards.clear();
    }
}

void ClientContext::afterReceivingTotal() {
    if (clientHand.previousTaken) {
        clientHand.previousTaken = false;
    } else {
        clientHand.previousScore = false;
    }

    clientHand.previousTotal = true;
    clientHand.countResults++;

    if (clientHand.countResults == 2) {
        clientHand.trickNumber = 1;
        clientHand.clientCards.clear();
    }
}

bool ClientContext::isClientAutomatic() {
    return isAutomatic;
}

void ClientContext::clearTakenTricks() {
    takenTricks.clear();
}

void ClientContext::appendTakenTrick(std::vector<Card> trickCards) {
    takenTricks.emplace_back(trickCards);
}

int ClientContext::getPollDescriptorAt(int index) {
    return pollDescriptors[index].fd;
}

int ClientContext::executePoll() {
    return poll(pollDescriptors, ClientConstants::CLIENT_CONNECTIONS,
                ClientConstants::CLIENT_TIMEOUT);
}

void ClientContext::setReadAt(int index) {
    pollDescriptors[index].events = POLLIN;
}

void ClientContext::appendUserRead(std::string message) {
    userBuffer.appendRead(message);
}

bool ClientContext::hasUserMessage() {
    return userBuffer.userMessageLen() > 0;
}

std::string ClientContext::popFirstUserMessage() {
    return userBuffer.popFirstUserMessage();
}

void ClientContext::appendServerRead(std::string message) {
    serverBuffer.appendRead(message);
}

bool ClientContext::hasServerMessage() {
    return serverBuffer.networkMessageLen() > 0;
}

std::string ClientContext::popFirstServerMessage() {
    return serverBuffer.popFirstNetworkMessage();
}

bool ClientContext::hasWriteMessage() {
    return writeBuffer.hasMessage();
}

std::string ClientContext::getFirstWriteMessage() {
    return writeBuffer.getFirstMessage();
}

std::string ClientContext::getCurrentWriteMessage() {
    return writeBuffer.getCurrentMessage();
}

bool ClientContext::wroteWholeWriteMessage(int sentLen) {
    return writeBuffer.wroteWholeMessage(sentLen);
}

ClientHand ClientContext::getClientHand() {
    return clientHand;
}

void ClientContext::setSentTrickTo(bool value) {
    clientHand.sentTrick = value;
}