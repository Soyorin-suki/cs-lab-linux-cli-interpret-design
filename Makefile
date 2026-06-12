BIN_DIR = bin
TARGET = user-sh

SRC := $(shell find src -name "*.cpp")

$(TARGET):
	mkdir -p $(BIN_DIR)
	g++ $(SRC) -Wall -o $(BIN_DIR)/user-sh

clean:
	rm -f $(TARGET)

run:
	./$(BIN_DIR)/$(TARGET)