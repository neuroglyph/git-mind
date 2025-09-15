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
	@python3 tools/docs/check_frontmatter.py
	@python3 tools/docs/check_docs.py --mode link
	@python3 tools/docs/check_docs.py --mode toc

.PHONY: md-verify
md-verify: md-lint

md-fix:
	@npx --yes -p markdownlint-cli2 markdownlint-cli2 --fix "**/*.md" "#.git" "#build"

# Changelog helpers
.PHONY: changelog-add
changelog-add:
	@python3 tools/changelog/add_entry.py -m "$(m)" $(if $(n),-n "$(n)")

.PHONY: all test clean docker-clean md-lint md-fix changelog-add
