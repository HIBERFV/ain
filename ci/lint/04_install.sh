#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

PATH=$PATH:~/.local/bin

curl -fsSL -o- https://bootstrap.pypa.io/pip/3.5/get-pip.py | python3
pip3 install codespell==2.2.1
pip3 install flake8==4.0.1
pip3 install mypy==0.942
pip3 install pyzmq==22.3.0
pip3 install vulture==2.3

SHELLCHECK_VERSION=v0.8.0
curl -L -s "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
export PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"
