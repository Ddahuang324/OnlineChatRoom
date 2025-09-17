#!/usr/bin/env bash
# Run or build+run a single test binary or GoogleTest case quickly.
# Usage:
#  ./scripts/run_single_test.sh -b build/tests/unit_tests -f MySuite.MyTest
#  ./scripts/run_single_test.sh -b build/tests/unit_tests --build-target tests
#  ./scripts/run_single_test.sh -b build/tests/unit_tests -f "MySuite.*"

set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
WORKSPACE_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$WORKSPACE_ROOT/build"
BINARY=""
BUILD_TARGET=""
GTEST_FILTER=""
JOBS=4

usage() {
  cat <<EOF
Usage: $0 -b <binary-path> [-f <gtest-filter>] [--build-target <cmake-target>] [-j <jobs>]

Options:
  -b|--binary        Path to test binary (relative to workspace or absolute)
  -f|--filter        GoogleTest filter (example: SuiteName.TestName or "Suite.*")
  --build-target     Optional: cmake target to build before running
  -j                 Parallel jobs when building (default: auto-detected)
  -h|--help          Show this help

Examples:
  # Run an existing test binary with filter
  $0 -b build/tests/unit_tests -f MySuite.MyTest

  # Build the CMake target then run
  $0 -b build/tests/unit_tests --build-target tests -f MySuite.*
EOF
}

# parse args
while [[ $# -gt 0 ]]; do
  case "$1" in
    -b|--binary)
      BINARY="$2"; shift 2;;
    -f|--filter)
      GTEST_FILTER="$2"; shift 2;;
    --build-target)
      BUILD_TARGET="$2"; shift 2;;
    -j)
      JOBS="$2"; shift 2;;
    -h|--help)
      usage; exit 0;;
    *)
      echo "Unknown arg: $1" >&2; usage; exit 1;;
  esac
done

if [[ -z "$BINARY" ]]; then
  echo "Error: --binary is required" >&2
  usage
  exit 2
fi

# detect default jobs
if [[ "$JOBS" == 4 ]]; then
  if command -v nproc >/dev/null 2>&1; then
    JOBS=$(nproc)
  else
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
  fi
fi

# resolve binary path
if [[ "$BINARY" != /* ]]; then
  BINARY="$WORKSPACE_ROOT/$BINARY"
fi

if [[ -n "$BUILD_TARGET" ]]; then
  echo "Building target '$BUILD_TARGET' in $BUILD_DIR (jobs=$JOBS)"
  cmake --build "$BUILD_DIR" --target "$BUILD_TARGET" -- -j "$JOBS"
fi

if [[ ! -x "$BINARY" ]]; then
  echo "Error: test binary not found or not executable: $BINARY" >&2
  exit 3
fi

echo "Running test binary: $BINARY"
if [[ -n "$GTEST_FILTER" ]]; then
  echo "  with GoogleTest filter: $GTEST_FILTER"
  "$BINARY" --gtest_filter="$GTEST_FILTER"
else
  "$BINARY"
fi
