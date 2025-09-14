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

docker-clean:
	./tools/docker-clean.sh

md-lint:
	@npx --yes -p markdownlint-cli2 markdownlint-cli2 "**/*.md" "#.git" "#build"

md-fix:
	@npx --yes -p markdownlint-cli2 markdownlint-cli2 --fix "**/*.md" "#.git" "#build"

.PHONY: all test clean docker-clean md-lint md-fix
