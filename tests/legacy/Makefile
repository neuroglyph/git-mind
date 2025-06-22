# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -I../../include -I../include
LDFLAGS = 

# Test targets
TESTS = test_edge_quality test_edge_behavior

all: $(TESTS)

test_edge_quality: test_edge_quality.c
	$(CC) $(CFLAGS) -o $@ $<

test_edge_behavior: test_edge_behavior.c ../src/edge/edge.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TESTS)
	@echo "🛡️  Running Quality Guardian Tests..."
	@./test_edge_quality
	@echo ""
	@echo "🧪 Running Behavioral Tests..."
	@./test_edge_behavior

clean:
	rm -f $(TESTS)

.PHONY: all run clean