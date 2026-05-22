#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# setup_tests.sh — Download the doctest header for the Gomoku project
#
# Usage:
#   chmod +x setup_tests.sh
#   ./setup_tests.sh
# ---------------------------------------------------------------------------

set -euo pipefail

DOCTEST_URL="https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h"
DOCTEST_DIR="external/doctest"
DOCTEST_HEADER="${DOCTEST_DIR}/doctest.h"

GREEN="\033[1;32m"
CYAN="\033[1;36m"
YELLOW="\033[1;33m"
NOC="\033[0m"

info()    { echo -e "${CYAN}[setup]${NOC} $*"; }
success() { echo -e "${GREEN}[done]${NOC}  $*"; }
warn()    { echo -e "${YELLOW}[skip]${NOC}  $*"; }

# ---------------------------------------------------------------------------
# Download doctest header
# ---------------------------------------------------------------------------
if [ -f "${DOCTEST_HEADER}" ]; then
    warn "${DOCTEST_HEADER} already exists — skipping download"
else
    info "Creating ${DOCTEST_DIR}/"
    mkdir -p "${DOCTEST_DIR}"

    info "Downloading doctest.h from GitHub..."
    if command -v curl &>/dev/null; then
        curl -fsSL "${DOCTEST_URL}" -o "${DOCTEST_HEADER}"
    elif command -v wget &>/dev/null; then
        wget -q "${DOCTEST_URL}" -O "${DOCTEST_HEADER}"
    else
        echo "ERROR: neither curl nor wget found. Install one and retry." >&2
        exit 1
    fi
    success "doctest.h saved to ${DOCTEST_HEADER}"
fi

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
echo ""
success "Setup complete. Next steps:"
echo "    make test        # compile test binary"
echo "    make run_tests   # compile + run all tests"
echo "    make run_tests FILTER='GameBoard*'  # filter by test name"
