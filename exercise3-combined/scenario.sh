#!/usr/bin/env bash
#
# Exercise 3 -- Defense in depth: combining
#   Layer 1: network-level MTD  (Exercise 1, URL/port shuffling)
#   Layer 2: syscall-level MTD  (Exercise 2, in-process syscall gate)
#
# Story: a web service hops across a secret port sequence (Layer 1), so an
# attacker who locks onto a fixed port loses the target most of the time. If the
# attacker nonetheless reaches code execution on a server, the process runs
# behind the in-process syscall gate (Layer 2), which blocks the shell-spawning
# execve. Two independent layers multiply the attacker's cost.
#
# Runs under Docker/Rosetta (Layer 2 uses the dynamic + inline-syscall binary).
#
set -uo pipefail
cd "$(dirname "$0")"
EX1="../exercise1-network-mtd"
EX2="../exercise2-syscall-mtd"

hr(){ printf '%.0s=' {1..70}; echo; }

# --- Build the Layer-2 syscall-MTD binary (exercise 2) --------------------
echo "[setup] building the syscall-MTD binary (exercise 2) ..."
if ! ( cd "$EX2" && make >/dev/null 2>&1 ); then
    echo "  (could not build victim.mtd -- Layer 2 step will be skipped;"
    echo "   run exercise 2 first, or use the Docker image where e9patch is prebuilt)"
fi
echo

# ==========================================================================
hr; echo "LAYER 1 -- Network-level MTD (URL / port shuffling)"; hr
echo "The service hops across a secret port sequence known to legitimate"
echo "clients. The attacker locks onto the initial port and keeps probing it."
echo

PORTS=(8123 9001 9002 9003 9004)
ATTACKER_PORT=${PORTS[0]}

get()  { curl -s --max-time 2 "localhost:$1"               2>/dev/null | grep -q 'This is a response'; }
move() { curl -s --max-time 2 "localhost:$1/restart?port=$2" >/dev/null 2>&1; }

python3 "$EX1/mtdnet.py" "${PORTS[0]}" >/tmp/ex3_srv.log 2>&1 &
SRV=$!
trap 'kill $SRV 2>/dev/null' EXIT
sleep 1.5

cur=${PORTS[0]}
legit_ok=0; atk_ok=0; rounds=${#PORTS[@]}
for i in "${!PORTS[@]}"; do
    target=${PORTS[$i]}
    if [ "$i" -ne 0 ]; then
        move "$cur" "$target"; cur=$target; sleep 1.2
    fi
    if get "$target"; then
        printf "  round %d: legit client -> :%s   OK\n"   "$i" "$target"; legit_ok=$((legit_ok+1))
    else
        printf "  round %d: legit client -> :%s   FAIL\n" "$i" "$target"
    fi
    if get "$ATTACKER_PORT"; then
        printf "           attacker    -> :%s   HIT\n"  "$ATTACKER_PORT"; atk_ok=$((atk_ok+1))
    else
        printf "           attacker    -> :%s   miss\n" "$ATTACKER_PORT"
    fi
done
echo
echo "  legitimate client success: ${legit_ok}/${rounds}  (follows the shuffle)"
echo "  attacker (fixed port)     : ${atk_ok}/${rounds}  -> network MTD shrank the attack window"
kill "$SRV" 2>/dev/null; trap - EXIT; sleep 0.3
echo

# ==========================================================================
hr; echo "LAYER 2 -- Syscall-level MTD (in-process gate, last line of defense)"; hr
echo "Suppose the attacker eventually lands code execution on a server and drops"
echo "a shell-spawning payload. The server process runs behind the syscall gate:"
echo
if [ -x "$EX2/victim.mtd" ]; then
    echo "  [no MTD]      \$ ./victim pwn"
    "$EX2/victim" pwn || true
    echo
    echo "  [syscall MTD] \$ ./victim.mtd pwn"
    "$EX2/victim.mtd" pwn; rc=$?
    echo "  (exit ${rc} -- 42 means the syscall gate blocked the shell)"
else
    echo "  (victim.mtd not built; run exercise 2 first)"
fi
echo

hr
echo "Defense in depth:"
echo "  Layer 1 (network MTD) multiplies the attacker's reconnaissance cost."
echo "  Layer 2 (syscall MTD) blocks the payload even after a break-in."
echo "  Combined, the mean time to compromise (MTTC) grows substantially."
hr
