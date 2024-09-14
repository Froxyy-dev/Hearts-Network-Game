#include "ClientPlayer.h"

void ClientPlayer::clientInitiate() {
    clientContext.initiateSending(getIamMessage(clientArguments));
}

void ClientPlayer::receiveBusy(std::string serverMessage) {
    if (not parseBusy(serverMessage, clientContext)) {
        return;
    }
}

void ClientPlayer::receiveDeal(std::string serverMessage) {
    if (not parseDeal(serverMessage, clientContext)) {
        return;
    }

    clientContext.clearTakenTricks();
    clientContext.afterReceivingDeal();
}

void ClientPlayer::receiveTrick(std::string serverMessage) {
    auto [isCorrect, placedCards] = parseTrickClient(serverMessage, clientContext);
    if (not isCorrect) {
        // An error occurred.
        return;
    }

    clientContext.afterReceivingTrick();

    // We need to send some message.
    if (not clientContext.isClientAutomatic()) {
        return;
    }

    std::string messageStr = strTrickClient(placedCards, clientContext.getClientHand());

    clientContext.initiateSending(messageStr);
}

void ClientPlayer::receiveTaken(std::string serverMessage) {
    std::vector<Card> trickCards = parseTaken(serverMessage, clientContext);
    if (trickCards.empty()) {
        return;
    }

    if (clientContext.tookLastTrick()) {
        clientContext.appendTakenTrick(trickCards);
    }

    clientContext.afterReceivingTaken();
}

void ClientPlayer::receiveWrong(std::string serverMessage) {
    parseWrong(serverMessage, clientContext);
}

void ClientPlayer::receiveScore(std::string serverMessage) {
    if (not parseResults(serverMessage, Messages::SCORE, clientContext)) {
        return;
    }

    clientContext.afterReceivingScore();
}

void ClientPlayer::receiveTotal(std::string serverMessage) {
    if (not parseResults(serverMessage, Messages::TOTAL, clientContext)) {
        return;
    }

    clientContext.afterReceivingTotal();
}

void ClientPlayer::handleMessageFromServer(std::string serverMessage) {
    if (clientContext.getClientHand().waitingForBusy() and canBeBusy(serverMessage)) {
        receiveBusy(serverMessage);
        return;
    }

    if (clientContext.getClientHand().waitingForDeal() and canBeDeal(serverMessage)) {
        receiveDeal(serverMessage);
        return;
    }

    if (clientContext.getClientHand().waitingForTrick() and canBeTrick(serverMessage)) {
        receiveTrick(serverMessage);
        return;
    }

    if (clientContext.getClientHand().waitingForTaken() and canBeTaken(serverMessage)) {
        receiveTaken(serverMessage);
        return;
    }

    if (clientContext.getClientHand().waitingForWrong() and canBeWrong(serverMessage)) {
        receiveWrong(serverMessage);
        return;
    }

    if (clientContext.getClientHand().waitingForScore() and canBeScore(serverMessage)) {
        receiveScore(serverMessage);
        return;
    }

    if (clientContext.getClientHand().waitingForTotal() and canBeTotal(serverMessage)) {
        receiveTotal(serverMessage);
        return;
    }
}

void ClientPlayer::handleMessageFromUser(const std::string &userMessage) {
    switch (userMessage[0]) {
    case '!': {
        if (not clientContext.getClientHand().waitingForTrick() or
            clientContext.hasWriteMessage() or not clientContext.clientCanSendTrick()) {
            std::cout << "You cannot send card now. " << Constants::AVAILABLE_COMMANDS
                      << std::flush;
            return;
        }

        Card cardToSend = Card();
        if (not setCardFromStr(cardToSend, userMessage.substr(1, userMessage.size() - 2))) {
            std::cout << "Entered wrong message. " << Constants::AVAILABLE_COMMANDS << std::flush;
            return;
        }

        std::string messageStr = cardToTrick(cardToSend, clientContext.getClientHand());
        clientContext.initiateSending(messageStr);

        break;
    }
    case 'c': {
        if (userMessage != ClientConstants::USER_CARDS) {
            std::cout << "Entered wrong message. " << Constants::AVAILABLE_COMMANDS << std::flush;
            break;
        }

        clientContext.displayCards();

        break;
    }
    case 't': {
        if (userMessage != ClientConstants::USER_TRICKS) {
            std::cout << "Entered wrong message. " << Constants::AVAILABLE_COMMANDS << std::flush;
            break;
        }

        clientContext.displayTricks();

        break;
    }
    default: {
        std::cout << "Entered wrong message. " << Constants::AVAILABLE_COMMANDS << std::flush;
        break;
    }
    }
}

void ClientPlayer::pollFromUser(char *buffer) {
    ssize_t readLen = read(clientContext.getPollDescriptorAt(ClientConstants::USER_INDEX), buffer,
                           ClientConstants::BUFFER_SIZE);
    if (readLen <= 0) {
        sysFatal("read");
    }

    std::string readMsg(buffer, readLen);
    clientContext.appendUserRead(readMsg);

    while (clientContext.hasUserMessage()) {
        std::string userMessage = clientContext.popFirstUserMessage();
        handleMessageFromUser(userMessage);
    }
}

int ClientPlayer::pollFromServer(char *buffer) {
    ssize_t readLen = read(clientContext.getPollDescriptorAt(ClientConstants::SERVER_INDEX), buffer,
                           ClientConstants::BUFFER_SIZE);

    if (readLen == 0 and clientContext.isGameFinished()) {
        return ClientConstants::GAME_FINISHED;
    }

    if (readLen == 0) {
        fatal("server disconnected");
    } else if (readLen < 0) {
        sysFatal("read");
    }

    std::string readMsg(buffer, readLen);
    clientContext.appendServerRead(readMsg);

    while (clientContext.hasServerMessage()) {
        std::string serverMessage = clientContext.popFirstServerMessage();
        clientContext.displayMessageFromServer(serverMessage);

        if (not clientContext.hasWriteMessage()) {
            handleMessageFromServer(serverMessage);
        }
    }

    return ClientConstants::GAME_NOT_FINISHED;
}

void ClientPlayer::writeToServer() {
    std::string message = clientContext.getFirstWriteMessage();
    ssize_t sentLen = clientContext.sendMessageClient(message);
    if (sentLen <= 0) {
        if (sentLen < 0 and (errno == EAGAIN or errno == EWOULDBLOCK)) {
            return;
        }

        sysFatal("write");
    }

    std::string currentMessage = clientContext.getCurrentWriteMessage();

    if (not clientContext.wroteWholeWriteMessage(sentLen)) {
        return;
    }

    clientContext.displayMessageFromClient(currentMessage);

    if (canBeTrick(currentMessage)) {
        clientContext.setSentTrickTo(true);
    }

    clientContext.setReadAt(ClientConstants::SERVER_INDEX);
}

ClientPlayer::ClientPlayer(int _socketFd, const ClientArguments &_clientArguments,
                           const std::string &_serverAddressStr,
                           const std::string &_clientAddressStr)
    : clientArguments(_clientArguments), serverAddressStr(_serverAddressStr),
      clientAddressStr(_clientAddressStr) {
    clientContext.createContext(_clientAddressStr, _serverAddressStr, _socketFd,
                                _clientArguments.isAutomatic, _clientArguments.tablePlace);
}

void ClientPlayer::handleGame() {
    // Client sends IAM.
    clientInitiate();

    static char buffer[ClientConstants::BUFFER_SIZE];

    while (true) {
        clientContext.resetRevents();

        int pollStatus = clientContext.executePoll();

        if (pollStatus == Constants::ERROR_CODE) {
            if (errno != EINTR) {
                sysFatal("poll");
            }

            continue;
        } // pollStatus > 0

        if (clientContext.pollReadFromUser()) {
            pollFromUser(buffer);
        }

        if (clientContext.pollReadFromServer()) {
            if (pollFromServer(buffer) == ClientConstants::GAME_FINISHED) {
                return;
            }
        }

        if (clientContext.pollWriteToServer()) {
            writeToServer();
        }
    }
}
