# Makefile -- in-process System-call MTD exercise (e9patch v1.0.1)
#
#   ./setup.sh      # once (local only): fetch & build e9patch into ./e9patch
#   make            # build victim + instrument it with the MTD hook
#   ./run_demo.sh   # run the full demonstration
#
# No ptrace: the MTD monitor runs inside the process (the e9patch hook), so it
# works under Docker Desktop's Rosetta 2 on Apple Silicon.
#
# The target is DYNAMICALLY linked on purpose: under Rosetta, instrumenting a
# statically linked glibc's early start-up syscalls crashes the translated
# trampolines. With dynamic linking, libc's syscalls stay in libc.so and only
# the program's own inline `syscall` instructions (the ones we monitor) are
# instrumented.

E9      ?= ./e9patch
CC      ?= gcc

TGTFLAGS = -O2 -Wall            # dynamically linked (default)

.PHONY: all clean check-e9
all: victim victim.mtd

# --- demo program ----------------------------------------------------------
victim: victim.c
	$(CC) $(TGTFLAGS) -o $@ $<

# --- e9patch instrumentation ----------------------------------------------
check-e9:
	@test -x "$(E9)/e9compile.sh" || { \
	  echo "ERROR: e9patch not found in $(E9). Run ./setup.sh first (Docker: prebuilt in /opt/e9patch)."; exit 1; }

# Compile the hook against e9patch's stdlib.c. (No -DNO_GLIBC: the target is
# dynamically linked, so the glibc-compatible instrumentation is correct.)
syscall_mtd: syscall_mtd.c check-e9
	$(E9)/e9compile.sh syscall_mtd.c -I $(E9)/examples

# Insert the MTD monitor before every `syscall` instruction of the target.
victim.mtd: victim syscall_mtd check-e9
	$(E9)/e9tool -M 'asm=/syscall/' -P 'before entry(state)@syscall_mtd' victim -o $@

clean:
	rm -f victim syscall_mtd syscall_mtd.o victim.mtd a.out
