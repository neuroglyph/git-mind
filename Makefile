# Makefile (tiny shim, lives at repo root)
.DEFAULT_GOAL := all
MESON_ARGS ?=
NINJA_ARGS ?=
builddir   ?= build

all:
	@meson setup $(builddir) $(MESON_ARGS) >/dev/null 2>&1 || true
	ninja -C $(builddir) $(NINJA_ARGS)

test:
	@meson setup $(builddir) $(MESON_ARGS) >/dev/null 2>&1 || true
	ninja -C $(builddir) test

clean:
	rm -rf $(builddir)

.PHONY: all test clean