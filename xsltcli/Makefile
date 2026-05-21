.PHONY: build clean

# Compiler and flags
CC = gcc
CFLAGS = `xml2-config --cflags` `xslt-config --cflags`
LIBS = `xml2-config --libs` `xslt-config --libs`
OUTPUT ?= xsltcli

# Build target
build: main.c
	$(CC) $(CFLAGS) -o $(OUTPUT) main.c $(LIBS)

# Clean target
clean:
	rm -f $(OUTPUT)

# Help target
help:
	@echo "Available targets:"
	@echo "  build  - Compile the xsltcli tool"
	@echo "  clean  - Remove compiled binary"
	@echo "  help   - Show this help message"
