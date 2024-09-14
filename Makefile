# Compiler and flags
CXX     = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++20 -Iinclude

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
CLIENT_SRC = $(wildcard $(SRC_DIR)/client/*.cpp) $(SRC_DIR)/kierki-klient.cpp
SERVER_SRC = $(wildcard $(SRC_DIR)/server/*.cpp) $(SRC_DIR)/kierki-serwer.cpp
COMMON_SRC = $(SRC_DIR)/common/common.cpp $(SRC_DIR)/err/err.cpp

# Object files
CLIENT_OBJ = $(patsubst $(SRC_DIR)/client/%.cpp,$(OBJ_DIR)/client/%.o,$(wildcard $(SRC_DIR)/client/*.cpp)) $(OBJ_DIR)/kierki-klient.o
SERVER_OBJ = $(patsubst $(SRC_DIR)/server/%.cpp,$(OBJ_DIR)/server/%.o,$(wildcard $(SRC_DIR)/server/*.cpp)) $(OBJ_DIR)/kierki-serwer.o
COMMON_OBJ = $(patsubst $(SRC_DIR)/common/%.cpp,$(OBJ_DIR)/common/%.o,$(SRC_DIR)/common/common.cpp) $(patsubst $(SRC_DIR)/err/%.cpp,$(OBJ_DIR)/err/%.o,$(SRC_DIR)/err/err.cpp)

# Targets
TARGETS = $(BIN_DIR)/kierki-klient $(BIN_DIR)/kierki-serwer

all: $(TARGETS)

# Linking rules
$(BIN_DIR)/kierki-klient: $(CLIENT_OBJ) $(COMMON_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN_DIR)/kierki-serwer: $(SERVER_OBJ) $(COMMON_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Pattern rules for object files
$(OBJ_DIR)/client/%.o: $(SRC_DIR)/client/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/server/%.o: $(SRC_DIR)/server/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/common/%.o: $(SRC_DIR)/common/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/err/%.o: $(SRC_DIR)/err/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Explicit rules for main .cpp files
$(OBJ_DIR)/kierki-klient.o: $(SRC_DIR)/kierki-klient.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/kierki-serwer.o: $(SRC_DIR)/kierki-serwer.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
