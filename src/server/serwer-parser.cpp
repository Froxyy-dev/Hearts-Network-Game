#include "server/serwer-parser.h"

static int readTimeout(char const *string) {
    if (strlen(string) > 7) {
        fatal("%s is not a valid timeout number", string);
    }

    char *endptr;
    errno = 0;
    unsigned long long timeout = strtoull(string, &endptr, 10);
    if (errno != 0 or *endptr != 0 or timeout * 1000 >= INT32_MAX) {
        fatal("%s is not a valid timeout number", string);
    }
    return (int)timeout;
}

/// @brief Function returns char from hand type.
static char handTypeToChar(HAND_TYPE handType) {
    if (handType != HAND_TYPE::UNDEFINED) {
        return static_cast<char>(static_cast<int>(handType) + '0');
    }
    return '?';
}

/// @brief Function set deal message for player at given tablePlace.
static void setDealStr(ServerHand &hand, TABLE_PLACE tablePlace) {
    std::string message = Messages::DEAL;
    message += handTypeToChar(hand.handType);
    message += tablePlaceToChar(static_cast<int>(hand.previousTrickTaker));
    message += getCardsStr(hand.playerCards[tablePlace]);
    message += Messages::END_OF_MESSAGE;

    hand.dealStrAtPlace[tablePlace] = message;
}

/// @brief Function reads game file provided by user.
static void parseGameFile(ServerArguments &serverArguments, ServerStatus &serverStatus) {
    std::ifstream gameFile(serverArguments.fileStr);
    if (not gameFile.is_open()) {
        sysFatal("cannot open file %s", serverArguments.fileStr);
    }

    std::string line;
    while (std::getline(gameFile, line)) {
        ServerHand currentHand = ServerHand();

        // First we get the hand type and first player.
        currentHand.handType = charToHandType(line[0]);
        currentHand.previousTrickTaker = charToTablePlace(line[1]);
        currentHand.currentClient = currentHand.previousTrickTaker;

        // Now we get the cards.
        for (auto placeChar : {'N', 'E', 'S', 'W'}) {
            TABLE_PLACE tablePlace = charToTablePlace(placeChar);

            std::getline(gameFile, line);
            currentHand.playerCards[tablePlace] = parseCardsVector(line, 0, line.size()).second;

            currentHand.playerScores[tablePlace] = 0;
            setDealStr(currentHand, tablePlace);
        }

        serverStatus.hands.emplace_back(currentHand);
    }
}

/// @brief Checks if client parameters are in proper form.
static void validateServerParameters(int argc, char **argv) {
    for (int i = 1; i < argc; i += 2) {
        std::string param(argv[i]);

        if (param.size() != 2 or param[0] != '-') {
            fatal("unknown option");
        }

        if (param[1] != 'p' and param[1] != 'f' and param[1] != 't') {
            fatal("unknown option -%c", param[1]);
        }

        if (i + 1 >= argc or argv[i + 1][0] == '-') {
            fatal("Parameter -%c is not followed by an argument", param[1]);
        }
    }
}

/// @brief Function parses arguments passed by user and reads game file.
void parseUserInput(int argc, char **argv, ServerArguments &serverArguments,
                    ServerStatus &serverStatus) {
    validateServerParameters(argc, argv);

    opterr = 0;
    int c;

    while ((c = getopt(argc, argv, "p:f:t:")) != -1)
        switch (c) {
        case 'p':
            serverArguments.portStr = optarg;
            break;
        case 'f':
            serverArguments.fileStr = optarg;
            break;
        case 't':
            serverArguments.timeoutStr = optarg;
            break;
        case '?':
            if (optopt == 'p' or optopt == 'f' or optopt == 't')
                fatal("Option -%c requires an argument.\n", optopt);
            if (isprint(optopt))
                fatal("Unknown option `-%c'.\n", optopt);
            fatal("Unknown option character `\\x%x'.\n", optopt);
        default:
            sysFatal("getopt");
        }

    if (serverArguments.fileStr == nullptr) {
        fatal("file required");
    }

    parseGameFile(serverArguments, serverStatus);

    if (serverArguments.timeoutStr != nullptr) {
        serverArguments.timeout = readTimeout(serverArguments.timeoutStr);
    }

    if (serverArguments.portStr != nullptr) {
        serverArguments.port = readPort(serverArguments.portStr);
    }
}
