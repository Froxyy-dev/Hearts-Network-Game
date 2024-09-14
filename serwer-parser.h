#ifndef KIERKI_SERWER_PARSER_H
#define KIERKI_SERWER_PARSER_H

#include <ctype.h>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "err.h"
#include "serwer-common.h"

void parseUserInput(int argc, char **argv, ServerArguments &serverArguments,
                    ServerStatus &server_status);

#endif // KIERKI_SERWER_PARSER_H
