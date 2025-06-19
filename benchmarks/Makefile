# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

CC = gcc
CFLAGS = -Wall -O3 -std=c11 `pkg-config --cflags libgit2` -I../include
LDFLAGS = `pkg-config --libs libgit2` -L/usr/local/lib -lroaring

# Source files needed for benchmark
BENCH_OBJS = wildebeest_stampede.o \
             ../build/obj/journal/writer.o \
             ../build/obj/journal/reader.o \
             ../build/obj/edge/cbor.o \
             ../build/obj/edge/edge.o \
             ../build/obj/edge/cbor_decode_ex.o \
             ../build/obj/cache/bitmap.o \
             ../build/obj/cache/builder.o \
             ../build/obj/cache/query.o \
             ../build/obj/cache/tree_builder.o \
             ../build/obj/cache/tree_size.o \
             ../build/obj/util/ulid.o \
             ../build/obj/util/sha.o \
             ../build/obj/util/error.o

all: stampede

stampede: wildebeest_stampede
	@echo "🦬 The stampede is ready!"
	@echo "Run: ./wildebeest_stampede"

wildebeest_stampede: $(BENCH_OBJS)
	$(CC) $(BENCH_OBJS) $(LDFLAGS) -o $@

wildebeest_stampede.o: wildebeest_stampede.c
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure main project is built first
../build/obj/%.o:
	@echo "Building main project first..."
	@(cd ../src && make)

run: stampede
	@echo "🦬⚡ STARTING THE STAMPEDE!"
	./wildebeest_stampede

clean:
	rm -f *.o wildebeest_stampede

.PHONY: all stampede run clean