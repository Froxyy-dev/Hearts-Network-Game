#ifndef KIERKI_KLIENT_PARSER_H
#define KIERKI_KLIENT_PARSER_H

#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client/klient-common.h"
#include "common/common.h"
#include "err/err.h"

void parseUserInput(int argc, char **argv, ClientArguments &clientArguments);

#endif // KIERKI_KLIENT_PARSER_H
