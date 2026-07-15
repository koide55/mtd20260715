#!/usr/bin/env bash
#
# run_demo.sh -- System-call-number MTD demonstration (e9patch v1.0.1).
#
# Shows two things:
#   1. A trusted, instrumented program (hello.mtd) runs normally under the
#      MTD tracer -- its KEY-shifted syscalls are translated back.
#   2. Un-instrumented code that issues a raw execve (evil, standing in for
#      injected shellcode) is DETECTED and BLOCKED by the tracer, even though
#      it spawns a shell when run directly.
#
set -uo pipefail
cd "$(dirname "$0")"

hr() { printf '%.0s-' {1..70}; echo; }

# Auto-build if needed. Inside Docker, e9patch is prebuilt at $E9 (=/opt/e9patch)
# so setup.sh is unnecessary; locally, run ./setup.sh first if e9patch is absent.
if [ ! -x ./mtd_tracer ] || [ ! -x ./hello.mtd ] || [ ! -x ./evil ]; then
    if [ -z "${E9:-}" ] && [ ! -x ./e9patch/e9tool ]; then
        echo "e9patch not found. Run ./setup.sh first (or use Docker: e9patch is prebuilt)."
        exit 1
    fi
    echo "[build] compiling tracer, demo programs and instrumented binaries ..."
    make || { echo "build failed"; exit 1; }
    echo
fi

hr; echo "1) Trusted program through the MTD tracer (should run normally):"; hr
./mtd_tracer ./hello.mtd

echo
hr; echo "2a) Un-instrumented 'shellcode' WITHOUT the tracer (attack succeeds):"; hr
./evil

echo
hr; echo "2b) Same 'shellcode' WITH the MTD tracer (attack detected & blocked):"; hr
./mtd_tracer ./evil
echo
echo "(exit $? from the tracer: non-zero means the intrusion was blocked.)"
