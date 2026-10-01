# ==============================================================================
# Makefile for TCP Header Data Modulation and Signaling - C Backend
# ==============================================================================

# Compiler and Flags
CC = C:/Dev-Cpp/bin/gcc.exe
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -O2 -s
BIN_DIR = bin
SRC_DIR = src
TEST_DIR = tests

# Core source modules
MODULES = $(SRC_DIR)/packet.c \
          $(SRC_DIR)/encoder.c \
          $(SRC_DIR)/error_control.c \
          $(SRC_DIR)/sequence_method.c \
          $(SRC_DIR)/window_method.c \
          $(SRC_DIR)/timing_method.c \
          $(SRC_DIR)/network_simulator.c \
          $(SRC_DIR)/comparison.c \
          $(SRC_DIR)/decoder.c \
          $(SRC_DIR)/verification.c \
          $(SRC_DIR)/unit_tests.c

TARGET = $(BIN_DIR)/backend_engine.exe
TEST_TARGET = $(BIN_DIR)/backend_test.exe

.PHONY: all run test clean help

all: $(TARGET)

$(TARGET): $(SRC_DIR)/main.c $(MODULES)
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/main.c $(MODULES)
	@echo [OK] Backend binary built: $(TARGET)

$(TEST_TARGET): $(TEST_DIR)/test_backend.c $(MODULES)
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_backend.c $(MODULES)
	@echo [OK] Test binary built: $(TEST_TARGET)

run: $(TARGET)
	@$(TARGET)

test: $(TARGET)
	@$(TARGET) --test

clean:
	@if exist $(BIN_DIR)\backend_engine.exe del /f /q $(BIN_DIR)\backend_engine.exe
	@if exist $(BIN_DIR)\backend_test.exe del /f /q $(BIN_DIR)\backend_test.exe
	@echo [OK] Cleaned build artifacts.

help:
	@echo Targets:
	@echo   make         - Build backend executable ($(TARGET))
	@echo   make run     - Build and run the backend demonstration
	@echo   make test    - Build and execute unit tests
	@echo   make clean   - Remove built binaries
