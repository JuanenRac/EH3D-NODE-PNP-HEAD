#!/usr/bin/env sh
# Runs the host tests and the tests of the tools; it never changes the version.
set -e
cd "$(dirname "$0")"
cmake -S tests -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
python3 -I -m unittest discover -s tools -p "test_*.py"
