#!/bin/bash
# Copyright 2026 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt
#
# Clones a Boost tree next to the checkout, drops this library into it, and
# installs the dependencies the benchmarks need. Exports BOOST_ROOT for the
# steps that follow. Only used by .github/workflows/benchmarks.yml.

set -e

LIBRARY=${GITHUB_REPOSITORY#*/}
REF=${GITHUB_BASE_REF:-$GITHUB_REF}
REF=${REF#refs/heads/}

BOOST_BRANCH=develop
if [ "$REF" = "master" ]; then
    BOOST_BRANCH=master
fi

PYTHON=python3
if ! command -v python3 > /dev/null 2>&1; then
    PYTHON=python
fi

cd ..
git clone -b "$BOOST_BRANCH" --depth 1 https://github.com/boostorg/boost.git boost-root
cd boost-root
mkdir -p "libs/$LIBRARY"

# Everything except doc/: it carries an Antora examples symlink that git-bash on
# Windows cannot recreate, plus a node_modules tree that nothing here needs.
for entry in "$GITHUB_WORKSPACE"/*; do
    if [ "$(basename "$entry")" != doc ]; then
        cp -r "$entry" "libs/$LIBRARY/"
    fi
done

git submodule update --init tools/boostdep
"$PYTHON" tools/boostdep/depinst/depinst.py --git_args "--jobs ${GIT_FETCH_JOBS:-4}" "$LIBRARY"

# The second comparator, header only. It goes inside the Boost tree so that the
# emulated jobs, which mount only that tree, see it too; run_benchmarks.sh adds
# it to the include path when it is there. WIDE_INTEGER_REF pins a commit so that
# the published numbers stay comparable from run to run.
git init -q wide-integer
git -C wide-integer fetch -q --depth 1 https://github.com/ckormanyos/wide-integer.git "${WIDE_INTEGER_REF:-master}"
git -C wide-integer checkout -q FETCH_HEAD

echo "BOOST_ROOT=$(pwd)" >> "$GITHUB_ENV"
