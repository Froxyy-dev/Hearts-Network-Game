#include "klient-parser.h"

/// @brief Returns true if character is client place.
static bool isClientPlace(char c) {
    return c == 'N' or c == 'E' or c == 'S' or c == 'W';
}

/// @brief Checks if client parameters are in proper form.
static void validateClientParameters(int argc, char **argv) {
    for (int i = 1; i < argc;) {
        std::string param(argv[i]);

        if (param.size() != 2 or param[0] != '-') {
            fatal("unknown option");
        }

        if (param[1] == '4' or param[1] == '6' or param[1] == 'a' or isClientPlace(param[1])) {
            i += 1;
            continue;
        }

        if (param[1] != 'h' and param[1] != 'p') {
            fatal("unknown option");
        }

        if (i + 1 >= argc or argv[i + 1][0] == '-') {
            fatal("Parameter -%c is not followed by an argument", param[1]);
        }

        i += 2;
    }
}

/// @brief Function parses user arguments.
void parseUserInput(int argc, char **argv, ClientArguments &clientArguments) {
    validateClientParameters(argc, argv);

    // We don't want getopt to print to stderr.
    opterr = 0;
    int c;

    while ((c = getopt(argc, argv, "h:p:46NESWa")) != Constants::ERROR_CODE)
        switch (c) {
        case 'h':
            clientArguments.host = optarg;
            break;
        case 'p':
            clientArguments.port = optarg;
            break;
        case '4':
            clientArguments.aiFamily = AF_INET;
            break;
        case '6':
            clientArguments.aiFamily = AF_INET6;
            break;
        case 'N':
        case 'E':
        case 'S':
        case 'W':
            clientArguments.setTablePlace((char)c);
            break;
        case 'a':
            clientArguments.isAutomatic = true;
            break;
        case '?':
            if (optopt == 'h' or optopt == 'p')
                fatal("Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fatal("Unknown option `-%c'.\n", optopt);
            else
                fatal("Unknown option character `\\x%x'.\n", optopt);
        default:
            sysFatal("getsockopt");
        }

    if (clientArguments.host == nullptr)
        fatal("host required");

    if (clientArguments.port == nullptr)
        fatal("port required");

    if (clientArguments.tablePlace == TABLE_PLACE::UNDEFINED) {
        fatal("place required");
    }
}
