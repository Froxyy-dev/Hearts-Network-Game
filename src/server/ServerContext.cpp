#include "server/ServerContext.h"

void ServerContext::createContext(int _baseTimeout, int _socketFd) {
    this->baseTimeout = _baseTimeout;
    this->socketTimeouts.assign(ServerConstants::CONNECTIONS, baseTimeout);
    this->waitingFor.assign(ServerConstants::CONNECTIONS, false);
    this->readBuffers.resize(ServerConstants::CONNECTIONS, ReadBuffer());
    this->writeBuffers.resize(ServerConstants::CONNECTIONS, WriteBuffer());
    this->clientAddressStr.resize(ServerConstants::CONNECTIONS);
    this->serverAddressStr.resize(ServerConstants::CONNECTIONS);
    this->socketFd = _socketFd;
    this->clientStates.resize(ServerConstants::CONNECTIONS, CLIENT_STATE::WAITING_FOR_START);

    storedPollEvents.resize(Constants::PLAYERS_NUMBER);

    initializePollStructures();
}

void ServerContext::initializePollStructures() {
    for (int index = 0; index < ServerConstants::CONNECTIONS; index++) {
        resetPollDescriptor(index);
    }

    pollDescriptors[ServerConstants::ACCEPT_INDEX].fd = socketFd;
    socketTimeouts[ServerConstants::ACCEPT_INDEX] = -1;
}

void ServerContext::resetPollDescriptor(const int index) {
    pollDescriptors[index].fd = -1;
    pollDescriptors[index].events = POLLIN;
    pollDescriptors[index].revents = 0;
}

bool ServerContext::isDescriptorReserved(int index) {
    return pollDescriptors[index].fd >= 0;
}

void ServerContext::closeDescriptor(int index) {
    if (isDescriptorReserved(index)) {
        close(pollDescriptors[index].fd);
    }
}

int ServerContext::getPollDescriptor(int index) {
    return pollDescriptors[index].fd;
}

bool ServerContext::pollReadAt(int index) {
    return isDescriptorReserved(index) and (pollDescriptors[index].revents & (POLLIN | POLLERR));
}

bool ServerContext::pollWriteAt(int index) {
    return isDescriptorReserved(index) and (pollDescriptors[index].revents & POLLOUT);
}

void ServerContext::pollSetWrite(int index) {
    pollDescriptors[index].events |= POLLOUT;
}

int ServerContext::getPollTimeout(int startingPoint) {
    int pollTimeout = -1;
    for (int i = startingPoint; i < ServerConstants::CONNECTIONS; i++) {
        if (not waitingFor[i] or pollDescriptors[i].events == 0)
            continue;

        if (pollTimeout == -1) {
            pollTimeout = socketTimeouts[i];
        } else {
            pollTimeout = std::min(pollTimeout, socketTimeouts[i]);
        }
    }

    return pollTimeout;
}

void ServerContext::resetRevents(int startingPoint) {
    for (int i = startingPoint; i < ServerConstants::CONNECTIONS; i++) {
        if (pollDescriptors[i].events != 0) {
            pollDescriptors[i].revents = 0;
        }
    }
}

void ServerContext::revaluateTimeouts(int duration, int startingPoint) {
    for (int i = startingPoint; i < (int)socketTimeouts.size(); i++) {
        if (i == ServerConstants::ACCEPT_INDEX or not waitingFor[i] or
            pollDescriptors[i].events == 0) {
            continue;
        }

        socketTimeouts[i] = std::max(socketTimeouts[i] - duration, 0);
    }
}

int ServerContext::executePoll(bool includePlayers) {
    int startingPoint = includePlayers ? 0 : ServerConstants::ACCEPT_INDEX;
    int timeout = getPollTimeout(startingPoint);

    resetRevents(startingPoint);

    auto start = std::chrono::high_resolution_clock::now();

    int pollStatus = poll(pollDescriptors + startingPoint,
                          ServerConstants::CONNECTIONS - startingPoint, timeout);
    if (pollStatus == -1) {
        return -1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    revaluateTimeouts(duration, startingPoint);

    return pollStatus;
}

void ServerContext::startWaitingFor(const int index) {
    waitingFor[index] = true;
}

void ServerContext::stopWaitingFor(const int index) {
    waitingFor[index] = false;
}

bool ServerContext::timeoutAt(int index) {
    return waitingFor[index] and socketTimeouts[index] == 0;
}

void ServerContext::resetTimeout(int index) {
    socketTimeouts[index] = baseTimeout;
}

void ServerContext::acceptConnection(int index, int clientFd, bool gameFull,
                                     const std::string &clientIP, const std::string &serverIp) {
    pollDescriptors[index].fd = clientFd;
    pollDescriptors[index].events = POLLIN;
    clientAddressStr[index] = clientIP;
    serverAddressStr[index] = serverIp;

    if (gameFull) {
        initiateSending(index, ServerConstants::GAME_FULL_MESSAGE, CLIENT_STATE::SENDING_BUSY);
        return;
    }

    clientStates[index] = CLIENT_STATE::WAITING_FOR_IAM;
    startWaitingFor(index);
}

void ServerContext::movePlayer(const int from, const int to) {
    pollDescriptors[to].fd = pollDescriptors[from].fd;

    clientAddressStr[to] = clientAddressStr[from];
    serverAddressStr[to] = serverAddressStr[from];

    readBuffers[to] = readBuffers[from];
    writeBuffers[to] = writeBuffers[from];

    clientStates[to] = CLIENT_STATE::SENDING_DEAL;

    closeConnection(from, false);
}

void ServerContext::closeConnection(int index, bool closeFd) {
    if (closeFd) {
        close(pollDescriptors[index].fd);
    }
    resetPollDescriptor(index);

    stopWaitingFor(index);
    resetTimeout(index);

    readBuffers[index] = ReadBuffer();
    writeBuffers[index] = WriteBuffer();

    clientAddressStr[index].clear();
    serverAddressStr[index].clear();
    clientStates[index] = CLIENT_STATE::EMPTY_PLACE;
}

void ServerContext::displayMessageFromClient(const int index, const std::string &message) {
    display(clientAddressStr[index], serverAddressStr[index], message);
}

void ServerContext::displayMessageFromServer(const int index, const std::string &message) {
    display(serverAddressStr[index], clientAddressStr[index], message);
}

ssize_t ServerContext::sendMessageServer(int index, std::string &message) {
    return sendMessage(pollDescriptors[index].fd, message.c_str(), message.size());
}

bool ServerContext::hasMessageFrom(const int index) {
    return readBuffers[index].networkMessageLen() > 0;
}

void ServerContext::checkIfEmpty(int index) {
    if (not writeBuffers[index].hasMessage()) {
        pollDescriptors[index].events = POLLIN;
    }
}

void ServerContext::initiateSending(int index, std::string message, CLIENT_STATE clientState) {
    if (clientState != CLIENT_STATE::SENDING_WRONG) {
        stopWaitingFor(index);
    }

    pollSetWrite(index);

    if (not message.empty()) {
        writeBuffers[index].appendMessage(message);
    }

    if (clientState != CLIENT_STATE::SENDING_WRONG) {
        clientStates[index] = clientState;
    }
}

bool ServerContext::hasEveryoneReceivedPreviousTaken() {
    for (int index = 0; index < ServerConstants::ACCEPT_INDEX; index++) {
        if (clientStates[index] == CLIENT_STATE::SENDING_PREVIOUS) {
            return false;
        }
    }
    return true;
}

bool ServerContext::inIndexes(const std::vector<int> &indexes, int index) {
    for (auto i : indexes) {
        if (index == i) {
            return true;
        }
    }

    return false;
}

void ServerContext::storeEventsExceptIndexes(std::vector<int> indexes) {
    storedIndexes = indexes;

    for (int i = 0; i < ServerConstants::ACCEPT_INDEX; i++) {
        if (inIndexes(indexes, i)) {
            continue;
        }

        storedPollEvents[i] = pollDescriptors[i].events;
        pollDescriptors[i].events = 0;
        pollDescriptors[i].revents = 0;
    }
}

void ServerContext::restoreEventsExceptIndexes() {
    for (int i = 0; i < ServerConstants::ACCEPT_INDEX; i++) {
        if (inIndexes(storedIndexes, i)) {
            continue;
        }

        pollDescriptors[i].events = storedPollEvents[i];
        storedPollEvents[i] = 0;
    }

    storedIndexes.clear();
}

void ServerContext::setClientStateAt(int index, CLIENT_STATE clientState) {
    clientStates[index] = clientState;
}

CLIENT_STATE ServerContext::getClientStateAt(int index) {
    return clientStates[index];
}

std::string ServerContext::getCurrentWriteMessageAt(int index) {
    return writeBuffers[index].getCurrentMessage();
}

std::string ServerContext::getFirstWriteMessageAt(int index) {
    return writeBuffers[index].getFirstMessage();
}

void ServerContext::appendMessageToWriteAt(int index, std::string message) {
    writeBuffers[index].appendMessage(message);
}

bool ServerContext::wroteWholeMessageAt(int index, int sentLen) {
    return writeBuffers[index].wroteWholeMessage(sentLen);
}

std::string ServerContext::popFirstReadMessageAt(int index) {
    return readBuffers[index].popFirstNetworkMessage();
}

void ServerContext::appendMessageToReadAt(int index, std::string message) {
    readBuffers[index].appendRead(message);
}
