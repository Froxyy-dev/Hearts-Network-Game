CXX     = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++20
TARGETS = kierki-klient kierki-serwer

all: $(TARGETS)

kierki-klient: kierki-klient.o err.o common.o klient-common.o klient-parser.o klient-communicator.o ClientPlayer.o ClientContext.o
	$(CXX) $(LDFLAGS) -o kierki-klient $^

kierki-serwer: kierki-serwer.o err.o common.o serwer-common.o serwer-parser.o serwer-communicator.o ServerCroupier.o ServerContext.o
	$(CXX) $(LDFLAGS) -o kierki-serwer $^

# To są zależności wygenerowane automatycznie za pomocą polecenia `g++ -MM *.cpp`.

ClientContext.o: ClientContext.cpp ClientContext.h common.h err.h \
 klient-common.h
ClientPlayer.o: ClientPlayer.cpp ClientPlayer.h ClientContext.h common.h \
 err.h klient-common.h klient-communicator.h
common.o: common.cpp common.h err.h
err.o: err.cpp err.h
kierki-klient.o: kierki-klient.cpp ClientPlayer.h ClientContext.h \
 common.h err.h klient-common.h klient-communicator.h klient-parser.h
kierki-serwer.o: kierki-serwer.cpp ServerCroupier.h ServerContext.h \
 common.h err.h serwer-common.h serwer-communicator.h serwer-parser.h
klient-common.o: klient-common.cpp klient-common.h common.h err.h
klient-communicator.o: klient-communicator.cpp klient-communicator.h \
 ClientContext.h common.h err.h klient-common.h
klient-parser.o: klient-parser.cpp klient-parser.h common.h err.h \
 klient-common.h
ServerContext.o: ServerContext.cpp ServerContext.h common.h err.h \
 serwer-common.h
ServerCroupier.o: ServerCroupier.cpp ServerCroupier.h ServerContext.h \
 common.h err.h serwer-common.h serwer-communicator.h
serwer-common.o: serwer-common.cpp serwer-common.h common.h err.h
serwer-communicator.o: serwer-communicator.cpp serwer-communicator.h \
 ServerContext.h common.h err.h serwer-common.h
serwer-parser.o: serwer-parser.cpp serwer-parser.h common.h err.h \
 serwer-common.h

clean:
	rm -f $(TARGETS) *.o