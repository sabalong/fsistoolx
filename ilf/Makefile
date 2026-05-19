# Makefile for ILF eval_server, eval_cli, eval_client, and evaltk_cli
# SPDX-License-Identifier: MIT
# Author: Kenny Karnama <kennykarnama@gmail.com>

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lm
UNAME_S := $(shell uname -s)

# C++ compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++11 -I./include

# Library flags
GLIB_CFLAGS = $(shell pkg-config --cflags glib-2.0)
GLIB_LIBS = $(shell pkg-config --libs glib-2.0)

XML2_CFLAGS = $(shell pkg-config --cflags libxml-2.0)
XML2_LIBS = $(shell pkg-config --libs libxml-2.0)

GSL_CFLAGS = $(shell pkg-config --cflags gsl)
GSL_LIBS = $(shell pkg-config --libs gsl)

TCC_PREFIX ?= $(shell brew --prefix tinycc 2>/dev/null || echo /usr/local)
TCC_CFLAGS = -I$(TCC_PREFIX)/include
ifeq ($(UNAME_S),Darwin)
TCC_LIBS = -L$(TCC_PREFIX)/lib -ltcc
else
TCC_LIBS = -L$(TCC_PREFIX)/lib -ltcc -ldl
endif

# Combine all flags
ALL_CFLAGS = $(CFLAGS) $(GLIB_CFLAGS) $(XML2_CFLAGS) $(GSL_CFLAGS) $(TCC_CFLAGS)
ALL_LIBS = $(GLIB_LIBS) $(XML2_LIBS) $(GSL_LIBS) $(TCC_LIBS) $(LDFLAGS)

# Directories
SRC_DIR = src
BUILD_DIR ?= build
TARGET_DIR ?= .

# Common source files (shared by eval_server and eval_cli)
COMMON_SOURCES = $(SRC_DIR)/eval.c \
                 $(SRC_DIR)/risk_profile_tree.c \
                 $(SRC_DIR)/kpmr_source_tree.c \
                 $(SRC_DIR)/parser.c \
                 $(SRC_DIR)/glibh.c \
                 $(SRC_DIR)/gslh.c \
                 $(SRC_DIR)/tcch.c \
                 $(SRC_DIR)/xmlh.c

# Common object files
COMMON_OBJECTS = $(COMMON_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# eval_server specific files
SERVER_SOURCES = $(SRC_DIR)/eval_server.c
SERVER_OBJECTS = $(SERVER_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
SERVER_TARGET = $(TARGET_DIR)/eval_server

# eval_cli specific files
CLI_SOURCES = $(SRC_DIR)/eval_cli.c
CLI_OBJECTS = $(CLI_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CLI_TARGET = $(TARGET_DIR)/eval_cli

# eval_client specific files
CLIENT_SOURCES = $(SRC_DIR)/eval_client.c
CLIENT_OBJECTS = $(CLIENT_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CLIENT_TARGET = $(TARGET_DIR)/eval_client

# evaltk_cli specific files (C++)
EVALTK_SOURCES = $(SRC_DIR)/evaltk_cli.cpp
EVALTK_OBJECTS = $(EVALTK_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
EVALTK_TARGET = $(TARGET_DIR)/evaltk_cli

# build_tree_cli specific files
BUILD_TREE_SOURCES = $(SRC_DIR)/build_tree_cli.c
BUILD_TREE_OBJECTS = $(BUILD_TREE_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
BUILD_TREE_TARGET = $(TARGET_DIR)/build_tree_cli

# kpmr_source_tree_cli specific files
KPMR_TREE_SOURCES = $(SRC_DIR)/kpmr_source_tree_cli.c
KPMR_TREE_OBJECTS = $(KPMR_TREE_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
KPMR_TREE_TARGET = $(TARGET_DIR)/kpmr_source_tree_cli

# build_tree_kpmr_cli specific files
BUILD_TREE_KPMR_SOURCES = $(SRC_DIR)/build_tree_kpmr_cli.c
BUILD_TREE_KPMR_OBJECTS = $(BUILD_TREE_KPMR_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
BUILD_TREE_KPMR_TARGET = $(TARGET_DIR)/build_tree_kpmr_cli

# src_inherent_cli specific files
SRC_INHERENT_SOURCES = $(SRC_DIR)/src_inherent_cli.c
SRC_INHERENT_OBJECTS = $(SRC_INHERENT_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
SRC_INHERENT_TARGET = $(TARGET_DIR)/src_inherent_cli

# Default target
.PHONY: all
all: $(SERVER_TARGET) $(CLI_TARGET) $(CLIENT_TARGET) $(EVALTK_TARGET) $(BUILD_TREE_TARGET) $(KPMR_TREE_TARGET) $(BUILD_TREE_KPMR_TARGET) $(SRC_INHERENT_TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

# Link object files to create eval_server executable
$(SERVER_TARGET): $(SERVER_OBJECTS) $(COMMON_OBJECTS) | $(TARGET_DIR)
	$(CC) $(SERVER_OBJECTS) $(COMMON_OBJECTS) $(ALL_LIBS) -o $(SERVER_TARGET)
	@echo "Build complete: $(SERVER_TARGET)"

# Link object files to create eval_cli executable
$(CLI_TARGET): $(CLI_OBJECTS) $(COMMON_OBJECTS) | $(TARGET_DIR)
	$(CC) $(CLI_OBJECTS) $(COMMON_OBJECTS) $(ALL_LIBS) -o $(CLI_TARGET)
	@echo "Build complete: $(CLI_TARGET)"

# Link object files to create eval_client executable
$(CLIENT_TARGET): $(CLIENT_OBJECTS) | $(TARGET_DIR)
	$(CC) $(CLIENT_OBJECTS) $(GLIB_LIBS) -o $(CLIENT_TARGET)
	@echo "Build complete: $(CLIENT_TARGET)"

# Link object files to create evaltk_cli executable
$(EVALTK_TARGET): $(EVALTK_OBJECTS) | $(TARGET_DIR)
	$(CXX) $(EVALTK_OBJECTS) -o $(EVALTK_TARGET)
	@echo "Build complete: $(EVALTK_TARGET)"

# Link object files to create build_tree_cli executable
$(BUILD_TREE_TARGET): $(BUILD_TREE_OBJECTS) $(COMMON_OBJECTS) | $(TARGET_DIR)
	$(CC) $(BUILD_TREE_OBJECTS) $(COMMON_OBJECTS) $(ALL_LIBS) -o $(BUILD_TREE_TARGET)
	@echo "Build complete: $(BUILD_TREE_TARGET)"

# Link object files to create kpmr_source_tree_cli executable
$(KPMR_TREE_TARGET): $(KPMR_TREE_OBJECTS) $(COMMON_OBJECTS) | $(TARGET_DIR)
	$(CC) $(KPMR_TREE_OBJECTS) $(COMMON_OBJECTS) $(ALL_LIBS) -o $(KPMR_TREE_TARGET)
	@echo "Build complete: $(KPMR_TREE_TARGET)"

# Link object files to create build_tree_kpmr_cli executable
$(BUILD_TREE_KPMR_TARGET): $(BUILD_TREE_KPMR_OBJECTS) $(BUILD_DIR)/xmlh.o $(BUILD_DIR)/glibh.o | $(TARGET_DIR)
	$(CC) $(BUILD_TREE_KPMR_OBJECTS) $(BUILD_DIR)/xmlh.o $(BUILD_DIR)/glibh.o $(GLIB_LIBS) $(XML2_LIBS) -o $(BUILD_TREE_KPMR_TARGET)
	@echo "Build complete: $(BUILD_TREE_KPMR_TARGET)"

# Link object files to create src_inherent_cli executable
$(SRC_INHERENT_TARGET): $(SRC_INHERENT_OBJECTS) $(COMMON_OBJECTS) | $(TARGET_DIR)
	$(CC) $(SRC_INHERENT_OBJECTS) $(COMMON_OBJECTS) $(ALL_LIBS) -o $(SRC_INHERENT_TARGET)
	@echo "Build complete: $(SRC_INHERENT_TARGET)"

# Compile C source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# Compile C++ source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(SERVER_TARGET) $(CLI_TARGET) $(CLIENT_TARGET) $(EVALTK_TARGET) $(BUILD_TREE_TARGET) $(KPMR_TREE_TARGET) $(BUILD_TREE_KPMR_TARGET) $(SRC_INHERENT_TARGET)
	@echo "Clean complete"

# Build only eval_server
.PHONY: eval_server
eval_server: $(SERVER_TARGET)

# Build only eval_cli
.PHONY: eval_cli
eval_cli: $(CLI_TARGET)

# Build only eval_client
.PHONY: eval_client
eval_client: $(CLIENT_TARGET)

# Build only evaltk_cli
.PHONY: evaltk_cli
evaltk_cli: $(EVALTK_TARGET)

# Build only build_tree_cli
.PHONY: build_tree_cli
build_tree_cli: $(BUILD_TREE_TARGET)

# Build only kpmr_source_tree_cli
.PHONY: kpmr_source_tree_cli
kpmr_source_tree_cli: $(KPMR_TREE_TARGET)

# Build only build_tree_kpmr_cli
.PHONY: build_tree_kpmr_cli
build_tree_kpmr_cli: $(BUILD_TREE_KPMR_TARGET)

# Build only src_inherent_cli
.PHONY: src_inherent_cli
src_inherent_cli: $(SRC_INHERENT_TARGET)

# Install targets (optional)
.PHONY: install
install: $(SERVER_TARGET) $(CLI_TARGET) $(CLIENT_TARGET) $(EVALTK_TARGET) $(BUILD_TREE_TARGET) $(KPMR_TREE_TARGET) $(SRC_INHERENT_TARGET)
	install -m 755 $(SERVER_TARGET) /usr/local/bin/
	install -m 755 $(CLI_TARGET) /usr/local/bin/
	install -m 755 $(CLIENT_TARGET) /usr/local/bin/
	install -m 755 $(EVALTK_TARGET) /usr/local/bin/
	install -m 755 $(BUILD_TREE_TARGET) /usr/local/bin/
	install -m 755 $(KPMR_TREE_TARGET) /usr/local/bin/
	install -m 755 $(BUILD_TREE_KPMR_TARGET) /usr/local/bin/
	install -m 755 $(SRC_INHERENT_TARGET) /usr/local/bin/

# Uninstall target (optional)
.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/$(SERVER_TARGET)
	rm -f /usr/local/bin/$(CLI_TARGET)
	rm -f /usr/local/bin/$(CLIENT_TARGET)
	rm -f /usr/local/bin/$(EVALTK_TARGET)
	rm -f /usr/local/bin/$(BUILD_TREE_TARGET)
	rm -f /usr/local/bin/$(KPMR_TREE_TARGET)
	rm -f /usr/local/bin/$(BUILD_TREE_KPMR_TARGET)
	rm -f /usr/local/bin/$(SRC_INHERENT_TARGET)

# Run the server
.PHONY: run
run: $(SERVER_TARGET)
	./$(SERVER_TARGET)

# Run the CLI (with example arguments - modify as needed)
.PHONY: run-cli
run-cli: $(CLI_TARGET)
	@echo "Usage: ./$(CLI_TARGET) --risk-profile <file> --sandi-source <file> --rating-to-score <file> --output <file>"

# Display help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all              - Build all executables (default)"
	@echo "  eval_server      - Build only eval_server"
	@echo "  eval_cli         - Build only eval_cli"
	@echo "  eval_client      - Build only eval_client"
	@echo "  evaltk_cli       - Build only evaltk_cli (expression evaluator)"
	@echo "  build_tree_cli   - Build only build_tree_cli (tree builder)"
	@echo "  kpmr_source_tree_cli - Build only kpmr_source_tree_cli (KPMR tree builder)"
	@echo "  build_tree_kpmr_cli - Build only build_tree_kpmr_cli (KPMR risk profile tree builder)"
	@echo "  src_inherent_cli - Build only src_inherent_cli (source inherent evaluator)"
	@echo "  clean            - Remove build artifacts"
	@echo "  install          - Install all executables to /usr/local/bin"
	@echo "  uninstall        - Remove all executables from /usr/local/bin"
	@echo "  run              - Build and run the server"
	@echo "  run-cli          - Show CLI usage information"
	@echo "  help             - Display this help message"

# Dependencies
$(BUILD_DIR)/eval_server.o: $(SRC_DIR)/eval_server.c $(SRC_DIR)/eval.h $(SRC_DIR)/risk_profile_tree.h
$(BUILD_DIR)/eval_cli.o: $(SRC_DIR)/eval_cli.c $(SRC_DIR)/eval.h $(SRC_DIR)/risk_profile_tree.h
$(BUILD_DIR)/eval_client.o: $(SRC_DIR)/eval_client.c
$(BUILD_DIR)/evaltk_cli.o: $(SRC_DIR)/evaltk_cli.cpp
$(BUILD_DIR)/src_inherent_cli.o: $(SRC_DIR)/src_inherent_cli.c $(SRC_DIR)/eval.h $(SRC_DIR)/hdr.h
$(BUILD_DIR)/build_tree_cli.o: $(SRC_DIR)/build_tree_cli.c $(SRC_DIR)/risk_profile_tree.h
$(BUILD_DIR)/build_tree_kpmr_cli.o: $(SRC_DIR)/build_tree_kpmr_cli.c
$(BUILD_DIR)/eval.o: $(SRC_DIR)/eval.c $(SRC_DIR)/eval.h $(SRC_DIR)/glibh.h $(SRC_DIR)/risk_profile_tree.h $(SRC_DIR)/tcch.h
$(BUILD_DIR)/risk_profile_tree.o: $(SRC_DIR)/risk_profile_tree.c $(SRC_DIR)/risk_profile_tree.h
$(BUILD_DIR)/parser.o: $(SRC_DIR)/parser.c $(SRC_DIR)/parser.h
$(BUILD_DIR)/glibh.o: $(SRC_DIR)/glibh.c $(SRC_DIR)/glibh.h
$(BUILD_DIR)/gslh.o: $(SRC_DIR)/gslh.c $(SRC_DIR)/gslh.h
$(BUILD_DIR)/tcch.o: $(SRC_DIR)/tcch.c $(SRC_DIR)/tcch.h $(SRC_DIR)/gslh.h $(SRC_DIR)/xmlh.h
$(BUILD_DIR)/xmlh.o: $(SRC_DIR)/xmlh.c $(SRC_DIR)/xmlh.h
