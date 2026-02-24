FROM node:22-slim

RUN apt-get update && apt-get install -y git python3 make g++ && rm -rf /var/lib/apt/lists/*

# Disable upgrade check — no network egress in test container
ENV GIT_MIND_DISABLE_UPGRADE_CHECK=1
ENV NO_COLOR=1

WORKDIR /app

# Install deps first (layer cache)
COPY package.json package-lock.json ./
RUN npm ci

# Copy source
COPY bin/ bin/
COPY src/ src/
COPY test/ test/
COPY docs/contracts/ docs/contracts/
COPY extensions/ extensions/

# Git identity for tests that init repos
RUN git config --global user.email "test@test.com" && \
    git config --global user.name "Test"

# Run only integration tests (files that spawn bin/git-mind.js)
CMD ["npx", "vitest", "run", "test/contracts.integration.test.js", "test/content.test.js", "test/version.test.js"]
