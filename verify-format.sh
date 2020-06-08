#!/bin/sh

PAGER=cat

# Use make format to cleanup the copied tree
if ! make format > /dev/null; then
    echo "Check if clang-format is installed!"
    exit 1
fi

# See if there are any changes compared to checked out sources.
if ! git diff --check --exit-code >/dev/null; then
    echo "Formatting incorrect for C files!"
    echo "Please run 'make format' before committing your changes,"
    echo "or manually apply the changes listed above."
    git diff
    exit 1
fi

echo "Formatting correct for C files."

PYFILES=$(find . -name '*.py')
PYEXTRA=

pycodestyle ${PYEXTRA} ${PYFILES}
RES=$?

if ! [ $RES -eq 0 ]; then
    echo "Formatting incorrect for Python files!"
    echo "Please manually fix the warnings listed above."
    exit 1
fi

echo "Formatting correct for Python files."
