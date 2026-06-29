CXX = g++
CXXFLAGS = -Wall -std=c++17 -g -MMD -MP

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin


TARGET = $(BIN_DIR)/user-sh
SRC := $(shell find src -name "*.cpp")
OBJ := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC))

DEPS := $(OBJ:.o=.d)

.PHONY: all clean run gdb

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p $(BIN_DIR)
	$(CXX) $(OBJ) -o $@
# 	g++ $(SRC) -Wall -o $(BIN_DIR)/user-sh

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: $(TARGET)
	./$(TARGET)


gdb: $(TARGET)
	gdb ./$(TARGET)


-include $(DEPS)

