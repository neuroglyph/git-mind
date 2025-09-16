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

.PHONY: ci-local
ci-local:
	@bash tools/ci/ci_local.sh

.PHONY: docs-verify
docs-verify:
	@python3 tools/docs/check_frontmatter.py
	@python3 tools/docs/check_docs.py --mode link
	@python3 tools/docs/check_docs.py --mode toc
	@python3 tools/docs/check_titles.py

.PHONY: header-compile
header-compile:
	@meson setup $(builddir) $(MESON_ARGS) >/dev/null 2>&1 || true
	@ninja -C $(builddir) header-compile

.PHONY: seed-review
# Usage: make seed-review PR=169 [OWNER=neuroglyph REPO=git-mind]
seed-review:
	@OWNER=$(OWNER); REPO=$(REPO); \
	 if [ -z "$$OWNER" ] || [ -z "$$REPO" ]; then \
	   url=$$(git remote get-url origin); \
	   case "$$url" in \
	     git@github.com:*) base=$${url#git@github.com:};; \
	     https://github.com/*) base=$${url#https://github.com/};; \
	     *) base="neuroglyph/git-mind";; \
	   esac; \
	   OWNER=$${base%%/*}; REPO=$${base##*/}; REPO=$${REPO%.git}; \
	 fi; \
	 if [ -z "$(PR)" ]; then echo "PR=<number> is required"; exit 2; fi; \
	 echo "Seeding review for $$OWNER/$$REPO PR $(PR)"; \
	 python3 tools/review/seed_feedback_from_github.py --owner $$OWNER --repo $$REPO --pr $(PR)

.PHONY: all test clean docker-clean md-lint md-fix changelog-add ci-local
