BIN_DIR = bin
TARGET = user-sh

SRC = src/main.cpp src/MyShell.cpp src/MyShell.hpp

$(TARGET):
	mkdir -p $(BIN_DIR)
	g++ $(SRC) -o $(BIN_DIR)/user-sh

clean:
	rm -f $(TARGET)

run:
	./$(BIN_DIR)/$(TARGET)