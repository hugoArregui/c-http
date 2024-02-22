CFLAGS=-Wall -Wextra -std=c17 -pedantic -ggdb -Wno-gnu-empty-initializer -D_DEFAULT_SOURCE
CFLAGS += -DNO_SSL
LIBS = -lpthread -lm

build:
	mkdir -p build/
	$(CC) $(CFLAGS) $(DEBUG) -o build/main src/main.c src/civetweb.c

format:
	clang-format --sort-includes -i src/main.c src/main.h

clean:
	rm -f build/*

.DEFAULT_GOAL := build
.PHONY: build
