BIN_DIR = bin
TARGET = user-sh

SRC = src/main.cpp src/MyShell.cpp src/MyShell.hpp src/Builtin.hpp src/Builtin.cpp src/Alias.cpp src/Alias.hpp

$(TARGET):
	mkdir -p $(BIN_DIR)
	g++ $(SRC) -Wall -o $(BIN_DIR)/user-sh

clean:
	rm -f $(TARGET)

run:
	./$(BIN_DIR)/$(TARGET)