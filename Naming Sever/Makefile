CC = /usr/bin/gcc
CFLAGS = -fdiagnostics-color=always -g
SRC_DIR = .
OBJ_DIR = obj
TARGET = NS
PORTS_TO_FREE = 8080 8081
GLOBAL_DEPS = External.c 

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

.PHONY: NS clean free_ports

all: $(TARGET) free_ports

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)


free_ports:
	@for port in $(PORTS_TO_FREE); do \
        echo "Releasing port $$port"; \
        lsof -ti :$$port | xargs -r kill; \
    done

clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(TARGET) 
	@if [ -n "$(log)" ] && [ "$(log)" = "true" ]; then \
		echo "Removing log file..."; \
    	rm -rf bookkeeping.log; \
	fi

logs:
	cat ./bookkeeping.log | less
