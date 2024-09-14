#ifndef KIERKI_KLIENT_PARSER_H
#define KIERKI_KLIENT_PARSER_H

#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "err.h"
#include "klient-common.h"

void parseUserInput(int argc, char **argv, ClientArguments &clientArguments);

#endif // KIERKI_KLIENT_PARSER_H
