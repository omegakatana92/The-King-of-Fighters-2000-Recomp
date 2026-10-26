#!/bin/sh
# Initialise the pinned neogeorecomp submodule and apply the generic toolkit
# fixes this title needs (MVS watchdog, KOF2000 fixed-layer banking, Z80 bank
# base). Safe to run more than once; it never resets an existing checkout.
set -eu
cd "$(dirname "$0")"

if [ ! -e neogeorecomp/.git ]; then
    git submodule update --init --recursive
fi
if [ ! -e neogeorecomp/ext/musashi/.git ]; then
    git -C neogeorecomp submodule update --init --recursive
fi

patch=$PWD/patches/neogeorecomp-kof2000.patch
if git -C neogeorecomp apply --check --reverse --ignore-whitespace "$patch" >/dev/null 2>&1; then
    echo "neogeorecomp: toolkit patch already applied"
elif git -C neogeorecomp apply --ignore-whitespace "$patch"; then
    echo "neogeorecomp: applied patches/neogeorecomp-kof2000.patch"
else
    echo "neogeorecomp: cannot apply the toolkit patch; reset the submodule and retry" >&2
    exit 1
fi
