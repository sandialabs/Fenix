#!/usr/bin/env bash

PRECOMMIT_VERSION="4.2.0"
PRECOMMIT_PYZ="pre-commit-${PRECOMMIT_VERSION}.pyz"
PRECOMMIT_URL="https://github.com/pre-commit/pre-commit/releases/download/v${PRECOMMIT_VERSION}/${PRECOMMIT_PYZ}"

if ! [[ -f "$PRECOMMIT_PYZ" ]]; then
    wget --no-verbose --show-progress "$PRECOMMIT_URL"
    chmod u+x "$PRECOMMIT_PYZ"
fi
PRECOMMIT=${PWD}/$PRECOMMIT_PYZ

#Go to git repo's root directory
cd $(git rev-parse --show-toplevel)

$PRECOMMIT install
