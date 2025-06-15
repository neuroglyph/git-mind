# SPDX-License-Identifier: Apache-2.0
# GitMind C Implementation Makefile

# Two modes: Direct compilation (inside Docker) or Docker orchestration
ifeq ($(DOCKER_BUILD),1)
    # Inside Docker - compilation mode
    include docker-guard.mk
    
    CC ?= cc
CFLAGS ?= -O3 -Wall -Wextra -Werror -pedantic -std=c99 -Iinclude -flto -Wstrict-prototypes -Wwrite-strings -Wshadow -Wformat=2
LDFLAGS ?= -flto

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    # Linux-specific flags (static linking possible)
    LDFLAGS += -static
endif
ifeq ($(UNAME_S),Darwin)
    # macOS doesn't support full static linking
    # Use -arch flags if provided
endif

# Windows/MinGW detection
ifeq ($(OS),Windows_NT)
    TARGET = git-mind.exe
else
    TARGET = git-mind
endif

SRCS = src/main.c src/gitmind.c src/link.c src/sha256.c src/sha1.c src/path.c src/check.c src/status.c src/traverse.c \
       src/orphan_ref.c src/ulid.c src/cbor.c src/fanout.c src/link_v2.c src/shell_utils.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
ifeq ($(UNAME_S),Darwin)
	strip $@
else
	strip --strip-all $@
endif

debug: CFLAGS = -g -O0 -Wall -Wextra -std=c99 -Iinclude -fsanitize=address,undefined
debug: LDFLAGS += -fsanitize=address,undefined
debug: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean debug test-binaries

# Test binaries
TEST_BINS = tests/test_holy_grail

test-binaries: $(TEST_BINS)

tests/test_holy_grail: tests/test_holy_grail.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LDFLAGS)
else
    # Outside Docker - orchestration mode
    include Makefile.docker
endif