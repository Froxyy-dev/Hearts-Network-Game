#ifndef KIERKI_SERWER_COMMUNICATOR_H
#define KIERKI_SERWER_COMMUNICATOR_H

#include <arpa/inet.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ServerContext.h"
#include "common.h"
#include "err.h"
#include "serwer-common.h"

TABLE_PLACE parseIam(const std::string &message);

std::string getBusyMessage(ServerContext &serverContext);

void setDealTakenMessage(TABLE_PLACE tablePlace, ServerStatus &serverStatus,
                         ServerContext &serverContext);
std::string getTrickMessage(ServerStatus &serverStatus);

bool canTrickBeParsed(std::string message);

bool parseTrickServer(std::string message, ServerStatus &server_status, TABLE_PLACE currentPlayer);

std::string getWrongMessage(ServerStatus &serverStatus);

std::string getResultsMessage(ServerStatus &serverStatus, const std::string &which);

#endif // KIERKI_SERWER_COMMUNICATOR_H
