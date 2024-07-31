CC=gcc
CFLAGS=-g -Wall
BIN_DIR=bin
SRC_DIR=src
INCLUDE_DIR=include
BUILD_DIR=build
LIBS = -lcurl
PORT?=8080  # Default port

all: $(BUILD_DIR)/proxy

# Create the build directory and copy the proxy executable
$(BUILD_DIR)/proxy: $(BIN_DIR)/proxy
	mkdir -p $(BUILD_DIR)
	cp $(BIN_DIR)/proxy $(BUILD_DIR)/proxy

# Link the object files into the final executable
$(BIN_DIR)/proxy: $(BIN_DIR)/http1.x_handler.o $(BIN_DIR)/proxy_server.o
	$(CC) $(CFLAGS) -o $(BIN_DIR)/proxy $(BIN_DIR)/http1.x_handler.o $(BIN_DIR)/proxy_server.o $(LIBS)

# Compile the source files into object files
$(BIN_DIR)/http1.x_handler.o: $(SRC_DIR)/http1.x_handler.c $(INCLUDE_DIR)/http1.x_handler.h
	$(CC) $(CFLAGS) -o $(BIN_DIR)/http1.x_handler.o -c $(SRC_DIR)/http1.x_handler.c

$(BIN_DIR)/proxy_server.o: $(SRC_DIR)/proxy_server.c $(INCLUDE_DIR)/http1.x_handler.h
	$(CC) $(CFLAGS) -o $(BIN_DIR)/proxy_server.o -c $(SRC_DIR)/proxy_server.c

clean:
	rm -f $(BIN_DIR)/* $(BUILD_DIR)/*

tar:
	tar -cvzf ass1.tgz $(SRC_DIR)/proxy_server.c README.md Makefile $(SRC_DIR)/http1.x_handler.c $(INCLUDE_DIR)/http1.x_handler.h

run: $(BUILD_DIR)/proxy
	$(BUILD_DIR)/proxy $(PORT)

.PHONY: all clean tar run
