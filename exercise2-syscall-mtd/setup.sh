#!/usr/bin/env bash
#
# setup.sh -- fetch and build e9patch v1.0.1 from source into ./e9patch.
#
# This replaces the old exercise's pre-provisioned AWS EC2 environment. Run
# once on an x86_64 Linux host. Requires: git, gcc, g++, make.
#
set -euo pipefail
cd "$(dirname "$0")"

E9_VERSION="v1.0.1"
E9_DIR="e9patch"
E9_REPO="https://github.com/GJDuck/e9patch"

# e9patch rewrites x86_64 ELF binaries; warn early on other architectures.
arch="$(uname -m)"
if [ "$arch" != "x86_64" ]; then
    echo "WARNING: host architecture is '$arch', but e9patch only supports x86_64."
    echo "         Run this exercise inside an x86_64 VM/container (or QEMU)."
    echo
fi

if [ -x "${E9_DIR}/e9tool" ] && [ -x "${E9_DIR}/e9patch" ]; then
    echo "[setup] e9patch already built in ./${E9_DIR} -- skipping."
    "${E9_DIR}/e9tool" --version 2>/dev/null | head -1 || true
    exit 0
fi

if [ ! -d "${E9_DIR}" ]; then
    echo "[setup] cloning e9patch ${E9_VERSION} ..."
    git clone --depth 1 --branch "${E9_VERSION}" "${E9_REPO}" "${E9_DIR}"
fi

echo "[setup] building e9patch (this takes ~1 minute) ..."
( cd "${E9_DIR}" && ./build.sh )

echo "[setup] done. Version:"
cat "${E9_DIR}/VERSION"
echo "[setup] tools: ${E9_DIR}/e9patch , ${E9_DIR}/e9tool"
