#include "ServerCroupier.h"

void ServerCroupier::closeConnectionWithPlayer(int index) {
    serverContext.closeConnection(index, true);
    serverStatus.activePlayers--;
    serverStatus.setDealSentAt(index, false);
}

int ServerCroupier::findFreeSlot() {
    int placeInPoll = Constants::ERROR_CODE;
    for (int index = ServerConstants::ACCEPT_INDEX + 1; index < ServerConstants::CONNECTIONS;
         ++index) {
        if (serverContext.isDescriptorReserved(index)) {
            continue;
        }

        placeInPoll = index;
        break;
    }

    return placeInPoll;
}

void ServerCroupier::prepareSendingWrong(int index) {
    std::string wrongMessage = getWrongMessage(serverStatus);
    serverContext.initiateSending(index, wrongMessage, CLIENT_STATE::SENDING_WRONG);
}

void ServerCroupier::prepareSendingBusy(int index) {
    std::string busyMessage = getBusyMessage(serverContext);
    serverContext.initiateSending(index, busyMessage, CLIENT_STATE::SENDING_BUSY);
}

void ServerCroupier::prepareSendingDeal(int index, CLIENT_STATE clientState) {
    auto client_table_place = static_cast<TABLE_PLACE>(index);

    if (serverStatus.dealSend[client_table_place]) {
        return;
    }

    setDealTakenMessage(client_table_place, serverStatus, serverContext);

    serverContext.initiateSending(index, "", clientState);
    serverStatus.setDealSentAt(index, true);
}

void ServerCroupier::afterSendingDeal(int index) {
    if (serverStatus.getCurrentTablePlace() == index) {
        prepareSendingTrick(index);
    } else {
        serverContext.setClientStateAt(index, CLIENT_STATE::WAITING_FOR_TURN);
    }
}

void ServerCroupier::prepareSendingTrick(int index) {
    std::string trickMessage = getTrickMessage(serverStatus);
    serverContext.initiateSending(index, trickMessage, CLIENT_STATE::SENDING_TRICK);
}

void ServerCroupier::afterSendingTrick(int index) {
    serverContext.startWaitingFor(index);
    serverContext.resetTimeout(index);
    serverContext.setClientStateAt(index, CLIENT_STATE::WAITING_FOR_TRICK);
}

void ServerCroupier::prepareSendingTaken() {
    int currentHand = serverStatus.currentHand;
    std::string takenStr = getTakenStr(serverStatus.hands[currentHand]);
    serverStatus.hands[currentHand].previousTaken.emplace_back(takenStr);

    serverStatus.setCurrentTablePlace(serverStatus.getPreviousTrickTaker());

    serverStatus.clearCardsFromTable();

    for (int i = 0; i < ServerConstants::ACCEPT_INDEX; i++) {
        serverContext.initiateSending(i, takenStr, CLIENT_STATE::SENDING_TAKEN);
    }
}

void ServerCroupier::afterSendingTaken(int index) {
    if (not canBeScore(serverContext.getCurrentWriteMessageAt(index))) {
        if (serverStatus.getCurrentTablePlace() == index) {
            prepareSendingTrick(index);
        } else {
            serverContext.setClientStateAt(index, CLIENT_STATE::WAITING_FOR_TURN);
        }
    } else {
        serverContext.setClientStateAt(index, CLIENT_STATE::SENDING_SCORE);
    }
}

void ServerCroupier::afterSendingPrevious(int index) {
    if (canBeTaken(serverContext.getCurrentWriteMessageAt(index))) {
        return;
    }

    serverContext.setClientStateAt(index, CLIENT_STATE::WAITING_FOR_TURN);
    if (serverContext.hasEveryoneReceivedPreviousTaken()) {
        serverContext.restoreEventsExceptIndexes();
    }

    if (serverStatus.getCurrentTablePlace() == index) {
        prepareSendingTrick(index);
    }
}

void ServerCroupier::prepareSendingScore(int index) {
    std::string resultsMessage = getResultsMessage(serverStatus, Messages::SCORE);
    serverContext.appendMessageToWriteAt(index, resultsMessage);
    serverStatus.updatePlayerTotalScore(index);
}

void ServerCroupier::afterSendingScore(int index) {
    serverContext.setClientStateAt(index, CLIENT_STATE::SENDING_TOTAL);
}

void ServerCroupier::prepareSendingTotal(int index) {
    std::string resultsMessage = getResultsMessage(serverStatus, Messages::TOTAL);
    serverContext.appendMessageToWriteAt(index, resultsMessage);
}

void ServerCroupier::afterSendingTotal(int index) {
    if (serverStatus.gameEnded) {
        serverStatus.alreadyLeft[static_cast<TABLE_PLACE>(index)] = true;

        closeConnectionWithPlayer(index);
        return;
    }

    serverStatus.dealSend[static_cast<TABLE_PLACE>(index)] = false;
    prepareSendingDeal(index, CLIENT_STATE::SENDING_DEAL);
}

void ServerCroupier::startClosingWaiting() {
    for (int index = ServerConstants::ACCEPT_INDEX + 1; index < ServerConstants::CONNECTIONS;
         index++) {
        if (serverContext.isDescriptorReserved(index)) {
            prepareSendingBusy(index);
        }
    }
}

TABLE_PLACE ServerCroupier::handleNonPlayerBuffer(int index) {
    bool alreadyHandledIam = false;
    TABLE_PLACE clientPlace = TABLE_PLACE::UNDEFINED;

    while (serverContext.hasMessageFrom(index)) {
        std::string clientMessage = serverContext.popFirstReadMessageAt(index);
        serverContext.displayMessageFromClient(index, clientMessage);

        if (serverContext.getClientStateAt(index) == CLIENT_STATE::SENDING_BUSY) {
            continue; // If we are sending busy we do not care about messages.
        }

        if (not alreadyHandledIam) { // First message should be IAM.
            clientPlace = parseIam(clientMessage);
            if (clientPlace == TABLE_PLACE::UNDEFINED) {
                serverContext.closeConnection(index, true);
                return TABLE_PLACE::UNDEFINED;
            }

            int clientPlaceInt = static_cast<int>(clientPlace);
            if (serverContext.isDescriptorReserved(clientPlaceInt)) {
                prepareSendingBusy(index);
                continue;
            }

            // If we are here that means we parsed IAM correctly.
            alreadyHandledIam = true;
        } else { // We already handled IAM message.
            if (not canTrickBeParsed(clientMessage)) {
                serverContext.closeConnection(index, true);
                return TABLE_PLACE::UNDEFINED;
            }

            prepareSendingWrong(index);
        }
    }

    if (not alreadyHandledIam) {
        return TABLE_PLACE::UNDEFINED;
    }

    return clientPlace;
}

void ServerCroupier::handleNonPlayerMessage(int index) {
    TABLE_PLACE clientPlace = handleNonPlayerBuffer(index);
    if (clientPlace == TABLE_PLACE::UNDEFINED) {
        return;
    }

    int clientPlaceInt = static_cast<int>(clientPlace);

    serverContext.movePlayer(index, clientPlaceInt);
    serverStatus.activePlayers++;

    if (serverStatus.gameStarted) {
        serverContext.setClientStateAt(clientPlaceInt, CLIENT_STATE::SENDING_PREVIOUS);
    }

    if (serverStatus.isGameActive()) {
        serverStatus.gameStarted = true;
        startClosingWaiting();

        std::vector<int> newPlayers;

        for (int i = 0; i < Constants::PLAYERS_NUMBER; i++) {
            prepareSendingDeal(i, serverContext.getClientStateAt(i));

            if (serverContext.getClientStateAt(i) == CLIENT_STATE::SENDING_PREVIOUS) {
                newPlayers.emplace_back(i);
            }
        }

        if (not newPlayers.empty()) {
            serverContext.storeEventsExceptIndexes(newPlayers);
        }
    }
}

void ServerCroupier::handleNonCurrentMessage(int index) {
    while (serverContext.hasMessageFrom(index)) {
        std::string clientMessage = serverContext.popFirstReadMessageAt(index);
        serverContext.displayMessageFromClient(index, clientMessage);

        if (not canTrickBeParsed(clientMessage)) {
            closeConnectionWithPlayer(index);
            return;
        }

        prepareSendingWrong(index);
    }
}

void ServerCroupier::afterReceivingTrick(int index) {
    serverContext.stopWaitingFor(index);
    serverContext.resetTimeout(index);
    serverContext.setClientStateAt(index, CLIENT_STATE::WAITING_FOR_TURN);

    int nextClientInt = (index + 1) % Constants::PLAYERS_NUMBER;
    auto nextClient = static_cast<TABLE_PLACE>(nextClientInt);

    serverStatus.setCurrentTablePlace(nextClient);

    // Server checks if it has received 4 cards.
    if (serverStatus.getPreviousTrickTaker() != nextClient) {
        // If server is communicating with next client, it will send trick after communication.
        if (serverContext.getClientStateAt(nextClientInt) != CLIENT_STATE::WAITING_FOR_TURN) {
            return;
        }

        // Otherwise we initiate sending trick message.
        prepareSendingTrick(nextClientInt);
        return;
    }

    prepareSendingTaken();

    serverStatus.finishTrick();
    if (not serverStatus.hasHandEnded()) {
        return;
    }

    for (int i = 0; i < ServerConstants::ACCEPT_INDEX; i++) {
        prepareSendingScore(i);
    }

    for (int i = 0; i < ServerConstants::ACCEPT_INDEX; i++) {
        prepareSendingTotal(i);
    }

    for (int i = 0; i < ServerConstants::ACCEPT_INDEX; i++) {
        serverStatus.setDealSentAt(i, false);
    }

    serverStatus.finishHand();
}

void ServerCroupier::handleCurrentMessage(int index) {
    auto currentPlayer = static_cast<TABLE_PLACE>(index);
    bool alreadyHandledTrick = false;

    while (serverContext.hasMessageFrom(index)) {
        std::string clientMessage = serverContext.popFirstReadMessageAt(index);
        serverContext.displayMessageFromClient(index, clientMessage);

        if (not alreadyHandledTrick and
            parseTrickServer(clientMessage, serverStatus, currentPlayer)) {
            alreadyHandledTrick = true;
            continue;
        }

        if (not canTrickBeParsed(clientMessage)) {
            closeConnectionWithPlayer(index);
            return;
        }

        prepareSendingWrong(index);
    }

    if (not alreadyHandledTrick) {
        return;
    }

    afterReceivingTrick(index);
}

void ServerCroupier::handlePlayersBuffer() {
    if (not serverStatus.pollIncludesPlayers()) {
        return;
    }

    for (int index = 0; index < ServerConstants::ACCEPT_INDEX; index++) {
        if (serverContext.getClientStateAt(index) == CLIENT_STATE::WAITING_FOR_TRICK) {
            handleCurrentMessage(index);
        } else {
            handleNonCurrentMessage(index);
        }
    }
}

void ServerCroupier::handleNewConnection() {
    if (not serverContext.pollReadAt(ServerConstants::ACCEPT_INDEX)) {
        return;
    }

    struct sockaddr_in6 clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    memset(&clientAddress, 0, clientAddressLen);

    int clientFd = accept(serverContext.getPollDescriptor(ServerConstants::ACCEPT_INDEX),
                          (struct sockaddr *)&clientAddress, &clientAddressLen);
    if (clientFd < 0) {
        sysError("accept");
        return;
    }

    if (fcntl(clientFd, F_SETFL, O_NONBLOCK)) {
        sysError("fcntl");
        return;
    }

    // Searching for a free slot.
    int placeInPoll;
    if ((placeInPoll = findFreeSlot()) == -1) {
        close(clientFd);
        return;
    }

    // Setting client ip.
    std::string clientIp = getIpv6AndPortAddress(clientAddress);

    struct sockaddr_in6 serverAddress;
    socklen_t serverAddressLen = sizeof(serverAddress);
    memset(&serverAddress, 0, serverAddressLen);
    // Find out what port the server is actually listening on.
    if (getsockname(clientFd, (struct sockaddr *)&serverAddress, &serverAddressLen) < 0) {
        sysError("getsockname");
        return;
    }

    std::string serverIp = getIpv6AndPortAddress(serverAddress);

    bool isGameFull = serverStatus.isGameActive() or serverStatus.gameEnded;
    serverContext.acceptConnection(placeInPoll, clientFd, isGameFull, clientIp, serverIp);
}

void ServerCroupier::handleTimeout() {
    for (int index = ServerConstants::ACCEPT_INDEX + 1; index < ServerConstants::CONNECTIONS;
         index++) {
        if (not serverContext.timeoutAt(index)) {
            continue;
        }
        // Server waited for IAM and timed out.
        serverContext.closeConnection(index, true);
    }

    if (not serverStatus.pollIncludesPlayers()) {
        return;
    }

    for (int index = 0; index < ServerConstants::ACCEPT_INDEX; index++) {
        if (not serverContext.timeoutAt(index)) {
            continue;
        } // Server waited for TRICK and timed out.

        prepareSendingTrick(index);
    }
}

void ServerCroupier::readFromNonPlayer(const int index, char *buffer) {
    // Server reads from new client.
    ssize_t readLen =
        read(serverContext.getPollDescriptor(index), buffer, ServerConstants::BUFFER_SIZE);

    if (readLen <= 0) {
        if (readLen < 0) {
            sysError("read");
        }

        serverContext.closeConnection(index, true);
        return;
    }

    std::string readMsg(buffer, readLen);
    serverContext.appendMessageToReadAt(index, readMsg);

    // SERVER parses IAM message.
    handleNonPlayerMessage(index);
}

void ServerCroupier::readFromNonPlayers(char *buffer) {
    for (int index = ServerConstants::ACCEPT_INDEX + 1; index < ServerConstants::CONNECTIONS;
         index++) {
        if (serverContext.pollReadAt(index)) {
            readFromNonPlayer(index, buffer);
        }
    }
}

void ServerCroupier::readFromPlayers(char *buffer) {
    if (not serverStatus.pollIncludesPlayers()) {
        return;
    }

    for (int index = 0; index < ServerConstants::ACCEPT_INDEX; index++) {
        if (not serverContext.pollReadAt(index)) {
            continue;
        }
        // There is something to read.

        ssize_t readLen =
            read(serverContext.getPollDescriptor(index), buffer, ServerConstants::BUFFER_SIZE);

        if (readLen <= 0) {
            if (readLen < 0) {
                sysError("read");
            }

            if (serverStatus.gameEnded) {
                serverStatus.alreadyLeft[static_cast<TABLE_PLACE>(index)] = true;
            }

            closeConnectionWithPlayer(index);
            continue;
        }

        std::string readMsg(buffer, readLen);
        serverContext.appendMessageToReadAt(index, readMsg);
    }
}

bool ServerCroupier::serverSentBusy(int index, ssize_t sentLen) {
    std::string message = serverContext.getCurrentWriteMessageAt(index);

    return serverContext.wroteWholeMessageAt(index, sentLen) and not canBeWrong(message);
}

void ServerCroupier::writeToNonPlayers() {
    for (int index = ServerConstants::ACCEPT_INDEX + 1; index < ServerConstants::CONNECTIONS;
         index++) {
        if (not serverContext.pollWriteAt(index)) {
            continue;
        }

        std::string message = serverContext.getFirstWriteMessageAt(index);
        ssize_t sentLen = serverContext.sendMessageServer(index, message);
        if (sentLen <= 0) {
            if (sentLen < 0 and (errno == EAGAIN or errno == EWOULDBLOCK)) {
                return;
            }

            serverContext.closeConnection(index, true);
        }

        if (serverSentBusy(index, sentLen)) {
            serverContext.displayMessageFromServer(index, message);
            serverContext.closeConnection(index, true);
        }
    }
}

void ServerCroupier::writeToPlayers() {
    if (not serverStatus.pollIncludesPlayers()) {
        return;
    }

    for (int index = 0; index < ServerConstants::ACCEPT_INDEX; index++) {
        if (not serverContext.pollWriteAt(index)) {
            continue;
        }

        std::string message = serverContext.getFirstWriteMessageAt(index);
        ssize_t sentLen = serverContext.sendMessageServer(index, message);
        if (sentLen <= 0) {
            if (sentLen < 0 and (errno == EAGAIN or errno == EWOULDBLOCK)) {
                continue;
            }

            if (serverStatus.gameEnded) {
                serverStatus.alreadyLeft[static_cast<TABLE_PLACE>(index)] = true;
            }

            closeConnectionWithPlayer(index);
            continue;
        }

        std::string currentMessage = serverContext.getCurrentWriteMessageAt(index);

        if (not serverContext.wroteWholeMessageAt(index, sentLen)) {
            continue;
        }

        serverContext.displayMessageFromServer(index, currentMessage);
        serverContext.checkIfEmpty(index);

        if (canBeWrong(currentMessage)) {
            continue;
        }

        if (serverContext.getClientStateAt(index) == CLIENT_STATE::SENDING_PREVIOUS) {
            afterSendingPrevious(index);
        } else if (canBeDeal(currentMessage)) {
            afterSendingDeal(index);
        } else if (canBeTrick(currentMessage)) {
            afterSendingTrick(index);
        } else if (canBeTaken(currentMessage)) {
            afterSendingTaken(index);
        } else if (canBeScore(currentMessage)) {
            afterSendingScore(index);
        } else if (canBeTotal(currentMessage)) {
            afterSendingTotal(index);
        }
    }
}

ServerCroupier::ServerCroupier(int socketFd, ServerArguments &serverArguments,
                               ServerStatus &serverStatus)
    : serverStatus(serverStatus) {
    int baseTimeout = serverArguments.timeout * 1000;
    serverContext.createContext(baseTimeout, socketFd);
}

void ServerCroupier::handleGame() {
    static char buffer[ServerConstants::BUFFER_SIZE];

    do {
        // Executing Poll.
        int pollStatus = serverContext.executePoll(serverStatus.pollIncludesPlayers());

        if (pollStatus == Constants::ERROR_CODE) {
            if (errno != EINTR) {
                sysFatal("poll");
            }

            continue;
        }

        if (pollStatus == 0) {
            handleTimeout();
            continue;
        }

        // Accept new client.
        handleNewConnection();

        // Read from non players.
        readFromNonPlayers(buffer);

        // Read from players.
        readFromPlayers(buffer);

        // Handle players' buffer.
        handlePlayersBuffer();

        // Write to non players.
        writeToNonPlayers();

        // Write to players.
        writeToPlayers();
    } while (not serverStatus.hasEveryoneLeft());

    // Server closes connections.
    for (int i = 0; i < ServerConstants::CONNECTIONS; i++) {
        serverContext.closeDescriptor(i);
    }
}
