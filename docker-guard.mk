# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective
# Docker build guard - ensures we're in Docker

ifndef DOCKER_BUILD
    $(error This Makefile must be run inside Docker. Use 'make' from the host)
endif

# Verify we're actually in a container
ifeq ($(shell test -f /.dockerenv && echo yes || echo no),yes)
    $(info ✅ Running inside Docker container)
else
    $(error ❌ DOCKER_BUILD is set but not in a container. Something is wrong!)
endif