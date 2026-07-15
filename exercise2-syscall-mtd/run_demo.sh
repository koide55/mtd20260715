#!/usr/bin/env bash
#
# run_demo.sh -- in-process System-call MTD demonstration (e9patch v1.0.1).
#
# Shows:
#   1. A trusted program runs normally with the MTD gate active (inline write
#      syscall is allowed).
#   2. The same program has a shell-spawning path (inline execve):
#        - uninstrumented + "pwn"  -> spawns a shell (attack succeeds)
#        - instrumented   + "pwn"  -> the execve is blocked at the syscall gate
#   3. The blocked syscall set is diversified per deployment (MTD_BLOCK).
#
# No ptrace: everything runs inside the process, so this works under Rosetta 2.
#
set -uo pipefail
cd "$(dirname "$0")"

hr() { printf '%.0s-' {1..70}; echo; }

if [ -z "${E9:-}" ] && [ ! -x ./e9patch/e9tool ]; then
    echo "e9patch not found. Run ./setup.sh first (Docker: e9patch is prebuilt)."
    exit 1
fi
echo "[build] make clean && make ..."
make clean >/dev/null 2>&1 || true
make || { echo "build failed"; exit 1; }
echo

hr; echo "1) Trusted program, benign run, MTD gate active (write allowed):"; hr
./victim.mtd

echo
hr; echo "2a) 'victim pwn' WITHOUT MTD (shell-spawning path succeeds):"; hr
./victim pwn

echo
hr; echo "2b) 'victim pwn' WITH the MTD syscall gate (execve blocked):"; hr
./victim.mtd pwn
rc=$?
echo "(exit ${rc} from victim.mtd: 42 means the MTD gate blocked the shell.)"

echo
hr; echo "3) Diversified policy (MTD_BLOCK): same binary, different gate per run."; hr
echo "   This run additionally blocks write(1), so even the benign inline write"
echo "   is stopped at the gate -- proving the policy is live and configurable:"
MTD_BLOCK="1,59,322" ./victim.mtd
echo "(different servers/runs can monitor different syscall sets -- a moving target.)"
