# Hearts-Network-Game

## Overview

Hearts-Network-Game is a network-based implementation of the popular card game **Hearts** (known as "kierki" in Polish), in which four players compete against each other. The game includes both server and client programs written in C/C++. The server manages the game logic, while each client represents a player who interacts with the server. Players can be either human-controlled or automated, and communication between the client and server is handled using TCP sockets.

## Features

- **Multiplayer functionality**: Up to four players can connect to the server.
- **Client-server architecture**: The game operates over a network using TCP sockets.
- **Automated and human players**: Players can be either automated using simple heuristics or human-controlled via a text-based user interface.
- **Rules enforcement**: The server ensures that all players follow the correct rules.
- **Multiple game types**: The game supports seven different types of rounds with varying scoring strategies.

## Game Rules

- The game is played with a standard 52-card deck, and there are four players: North (N), East (E), South (S), and West (W).
- Each player is dealt 13 cards. The game consists of 13 tricks.
- Players must follow the suit of the leading card if possible. If a player does not have a card of that suit, they may play any card.
- The player with the highest card of the leading suit wins the trick and leads the next round.
- The goal is to minimize the number of points earned. Points are assigned based on specific cards or tricks taken during the game.

The game supports multiple types of rounds:
1. Avoid taking tricks.
2. Avoid taking Hearts.
3. Avoid taking Queens.
4. Avoid taking face cards (Jacks and Kings).
5. Avoid taking the King of Hearts.
6. Avoid taking the 7th or 13th trick.
7. "Robber" mode: points are given for everything mentioned above.

## Communication Protocol

The server and clients communicate using TCP sockets and exchange ASCII messages. Each message is terminated with a `\r\n` sequence.

### Server Messages:
- `BUSY<places>`: Informs the client that certain seats are already taken.
- `DEAL<type><starter><cards>`: Informs the client of the current deal, including the player's cards.
- `TRICK<number><cards>`: Asks the client to play a card for the current trick.
- `TAKEN<number><cards><winner>`: Informs clients which player took the current trick.
- `SCORE<N><points><E><points><S><points><W><points>`: Provides the scores for the current round.
- `TOTAL<N><points><E><points><S><points><W><points>`: Provides the cumulative scores for the entire game.

### Client Messages:
- `IAM<position>`: Sent by the client after connecting to indicate which seat they want to take.
- `TRICK<card>`: Sent by the client to play a card for the current trick.

## Client Interface

- **Manual Play**: The client provides a text-based interface for users to view their cards and input moves.
- **Automated Play**: The client can operate in an automated mode where decisions are made heuristically.

### Commands:
- `cards`: Display the current cards in hand.
- `tricks`: Display the tricks taken so far.
- `!<card>`: Play a specific card (e.g., `!10C` for the 10 of clubs).

## Project Structure
```
project/
├── src/
│   ├── client/
│   │   ├── ClientContext.cpp
│   │   ├── ClientPlayer.cpp
│   │   ├── klient-common.cpp
│   │   ├── klient-communicator.cpp
│   │   ├── klient-parser.cpp
│   ├── server/
│   │   ├── ServerContext.cpp
│   │   ├── ServerCroupier.cpp
│   │   ├── serwer-common.cpp
│   │   ├── serwer-communicator.cpp
│   │   ├── serwer-parser.cpp
│   ├── common/
│   │   ├── common.cpp
│   ├── err/
│   │   ├── err.cpp
│   ├── kierki-klient.cpp
│   └── kierki-serwer.cpp
├── include/
│   ├── client/
│   │   ├── ClientContext.h
│   │   ├── ClientPlayer.h
│   │   ├── klient-common.h
│   │   ├── klient-communicator.h
│   │   ├── klient-parser.h
│   ├── server/
│   │   ├── ServerContext.h
│   │   ├── ServerCroupier.h
│   │   ├── serwer-common.h
│   │   ├── serwer-communicator.h
│   │   ├── serwer-parser.h
│   ├── common/
│   │   ├── common.h
│   └── err/
│       └── err.h
├── bin/
│   ├── kierki-klient
│   ├── kierki-serwer
├── LICENSE
├── README.md
└── Makefile
```


## Building and Running

### Prerequisites:
- Linux system with GCC or Clang.
- Makefile and basic C/C++ knowledge.

### Compilation

To compile both the server and client, run:

```bash
make
```

This will create two binaries in bin/ directory: `kierki-serwer` and `kierki-klient`.

### Running the Server

```bash
./bin/kierki-serwer -f <game-definition-file> [-p <port>] [-t <timeout>]
```

- `-f`: Specifies the game definition file.
- `-p`: Specifies the port (optional).
- `-t`: Sets the timeout (default: 5 seconds).

### Running the Client

```bash
./bin/kierki-klient -h <host> -p <port> -N/E/S/W [-4/-6] [-a]
```

- `-h`: Specifies the server IP or hostname.
- `-p`: Specifies the port.
- `-N/E/S/W`: Selects the player's position at the table.
- `-4` or `-6`: Forces IPv4 or IPv6 (optional).
- `-a`: Runs the client in automated mode (optional).

## License

This project is distributed under the MIT License.

