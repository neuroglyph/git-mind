# SPDX-License-Identifier: Apache-2.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -O2 `pkg-config --cflags libgit2`
LDFLAGS = `pkg-config --libs libgit2` -L/usr/local/lib -lroaring

# Directories
SRCDIR = .
OBJDIR = ../build/obj
BINDIR = ../build/bin
INCDIR = ../include

# Source files (organized by module)
JOURNAL_SRCS = journal/writer.c journal/reader.c
EDGE_SRCS = edge/cbor.c edge/edge.c
CACHE_SRCS = cache/bitmap.c cache/builder.c cache/query.c
CLI_SRCS = cli/main.c cli/link.c cli/list.c
UTIL_SRCS = util/ulid.c util/sha.c util/error.c

ALL_SRCS = $(JOURNAL_SRCS) $(EDGE_SRCS) $(CACHE_SRCS) $(CLI_SRCS) $(UTIL_SRCS)
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(ALL_SRCS))

# Main target
all: $(BINDIR)/git-mind

$(BINDIR)/git-mind: $(OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean