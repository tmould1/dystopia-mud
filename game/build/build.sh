#!/bin/bash
# GCC build script for Dystopia MUD (Linux)
# Run from game/build directory
#
# Usage:
#   ./build.sh              - Build with parallel compilation
#   ./build.sh clean        - Remove build artifacts

set -e

# Stay in build directory (where this script lives)
cd "$(dirname "$0")"

# Handle 'clean' argument
if [ "${1}" = "clean" ]; then
    echo "Cleaning build artifacts..."
    make -f Makefile clean 2>/dev/null || true
    echo "Clean complete."
    exit 0
fi

# Create logs directory if it doesn't exist
mkdir -p logs

# Create gamedata directory if it doesn't exist (output location)
mkdir -p ../../gamedata

# Detect number of CPU cores for parallel build
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Generate timestamp for log file
LOGFILE="logs/build_$(date +%Y%m%d-%H%M%S).log"

# Run make with parallel jobs and log output
echo "Building Dystopia MUD with ${JOBS} parallel jobs... (logging to ${LOGFILE})"
make -j"${JOBS}" -f Makefile "$@" 2>&1 | tee "${LOGFILE}"

BUILD_STATUS=${PIPESTATUS[0]}

if [ ${BUILD_STATUS} -ne 0 ]; then
    echo ""
    echo "BUILD FAILED - see ${LOGFILE} for details"
    exit 1
fi

echo ""
echo "Build successful."
echo "Build log saved to: ${LOGFILE}"
