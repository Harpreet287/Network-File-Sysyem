GLOBAL_DEPS_SRC = ..
GLOBAL_DEPS = Externals.c 
CC = /usr/bin/gcc
CFLAGS = -fdiagnostics-color=always -g
SRC_DIR = .
OBJ_DIR = obj
TARGET = Client
LOG_FILE = Clientlog.log

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

.PHONY: clean 

all: $(TARGET) 
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(GLOBAL_DEPS_SRC)/$(GLOBAL_DEPS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)


clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(TARGET) 
	@if [ -n "$(log)" ] && [ "$(log)" = "true" ]; then \
		echo "Removing log file..."; \
    	rm -rf $(LOG_FILE); \
	fi

logs:
	cat $(LOG_FILE) | less