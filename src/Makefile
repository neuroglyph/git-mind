# SPDX-License-Identifier: Apache-2.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 `pkg-config --cflags libgit2`
LDFLAGS = `pkg-config --libs libgit2` -L/usr/local/lib -lroaring

# Directories
SRCDIR = .
OBJDIR = ../build/obj
BINDIR = ../build/bin
INCDIR = ../include

# Source files (organized by module)
JOURNAL_SRCS = journal/writer.c journal/reader.c
EDGE_SRCS = edge/cbor.c edge/edge.c edge/cbor_decode_ex.c
CACHE_SRCS = cache/bitmap.c cache/builder.c cache/query.c cache/tree_builder.c cache/tree_size.c
CLI_SRCS = cli/main.c cli/link.c cli/list.c cli/install_hooks.c cli/cache_rebuild.c
UTIL_SRCS = util/ulid.c util/sha.c util/error.c
HOOK_SRCS = hooks/augment.c

ALL_SRCS = $(JOURNAL_SRCS) $(EDGE_SRCS) $(CACHE_SRCS) $(CLI_SRCS) $(UTIL_SRCS)
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(ALL_SRCS))

# Hook objects (separate binary)
HOOK_OBJS = $(OBJDIR)/hooks/post-commit.o $(OBJDIR)/hooks/augment.o \
            $(OBJDIR)/journal/writer.o $(OBJDIR)/journal/reader.o \
            $(OBJDIR)/edge/cbor.o $(OBJDIR)/edge/edge.o $(OBJDIR)/edge/cbor_decode_ex.o \
            $(OBJDIR)/util/ulid.o $(OBJDIR)/util/sha.o $(OBJDIR)/util/error.o

# Main target
all: $(BINDIR)/git-mind $(BINDIR)/git-mind-hook

$(BINDIR)/git-mind: $(OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BINDIR)/git-mind-hook: $(HOOK_OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(HOOK_OBJS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean