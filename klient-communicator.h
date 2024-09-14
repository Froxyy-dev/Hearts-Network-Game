#ifndef KIERKI_KLIENT_COMMUNICATOR_H
#define KIERKI_KLIENT_COMMUNICATOR_H

#include <arpa/inet.h>
#include <cassert>
#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ClientContext.h"
#include "common.h"
#include "err.h"
#include "klient-common.h"

std::string getIamMessage(ClientArguments client_arguments);

bool parseBusy(const std::string &message, ClientContext &clientContext);

bool parseDeal(std::string message, ClientContext &clientContext);

std::pair<bool, std::vector<Card>> parseTrickClient(std::string message,
                                                    ClientContext &clientContext);

std::vector<Card> parseTaken(std::string message, ClientContext &clientContext);

bool parseWrong(std::string message, ClientContext &clientContext);

bool parseResults(std::string message, const std::string &expected, ClientContext &clientContext);

#endif // KIERKI_KLIENT_COMMUNICATOR_H
