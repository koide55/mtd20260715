#!/usr/bin/env bash
#
# Exercise 1 demo: network-level MTD (port shuffling).
# Starts mtdnet.py, then walks the target across three ports with curl.
#
set -euo pipefail
cd "$(dirname "$0")"

P0=8123 P1=9999 P2=8111

echo "[demo] starting MTD web app on port ${P0} ..."
python3 mtdnet.py "${P0}" >/tmp/mtdnet.log 2>&1 &
PID=$!
trap 'kill "${PID}" 2>/dev/null || true' EXIT
sleep 1.5

show() { curl -s --max-time 2 "$1" | grep -o 'cnt=[0-9]*' || echo "(no response)"; }

echo "[demo] GET :${P0}           -> $(show localhost:${P0})"
echo "[demo] GET :${P0}/restart?port=${P1} -> $(show "localhost:${P0}/restart?port=${P1}")"
sleep 1.5
echo "[demo] GET :${P0} (moved)   -> $(curl -s --max-time 2 localhost:${P0} 2>/dev/null || echo 'CONNECTION REFUSED (target has moved)')"
echo "[demo] GET :${P1}           -> $(show localhost:${P1})"
echo "[demo] GET :${P1}/restart?port=${P2} -> $(show "localhost:${P1}/restart?port=${P2}")"
sleep 1.5
echo "[demo] GET :${P2}           -> $(show localhost:${P2})"
echo "[demo] done. The listening port kept moving 8123 -> 9999 -> 8111."
