# Makefile -- in-process System-call MTD exercise (e9patch v1.0.1)
#
#   ./setup.sh      # once (local only): fetch & build e9patch into ./e9patch
#   make            # build demo programs + instrument them with the MTD hook
#   ./run_demo.sh   # run the full demonstration
#
# No ptrace: the MTD monitor runs entirely inside the process (the e9patch
# hook), so this works under Docker Desktop's Rosetta 2 on Apple Silicon.
# e9patch is x86_64-only, so the image/host must be x86_64 (Rosetta emulated ok).

E9      ?= ./e9patch
CC      ?= gcc

# Static + non-PIE: keeps all syscall sites inside the binary so e9patch can
# instrument them (100% coverage) and there is no dynamic loader.
TGTFLAGS = -static -no-pie -O2 -Wall

.PHONY: all clean check-e9
all: hello hello.mtd victim victim.mtd

# --- demo programs ---------------------------------------------------------
hello: hello.c
	$(CC) $(TGTFLAGS) -o $@ $<

victim: victim.c
	$(CC) $(TGTFLAGS) -o $@ $<

# --- e9patch instrumentation ----------------------------------------------
check-e9:
	@test -x "$(E9)/e9compile.sh" || { \
	  echo "ERROR: e9patch not found in $(E9). Run ./setup.sh first (Docker: prebuilt in /opt/e9patch)."; exit 1; }

# Compile the hook against e9patch's stdlib.c. -DNO_GLIBC=1 is REQUIRED because
# the targets are statically linked (no glibc TLS for the instrumentation).
syscall_mtd: syscall_mtd.c check-e9
	$(E9)/e9compile.sh syscall_mtd.c -I $(E9)/examples -DNO_GLIBC=1

# Insert the MTD monitor before every `syscall` instruction of the target.
hello.mtd: hello syscall_mtd check-e9
	$(E9)/e9tool -M 'asm=/syscall/' -P 'before entry(state)@syscall_mtd' hello -o $@

victim.mtd: victim syscall_mtd check-e9
	$(E9)/e9tool -M 'asm=/syscall/' -P 'before entry(state)@syscall_mtd' victim -o $@

clean:
	rm -f hello victim syscall_mtd syscall_mtd.o hello.mtd victim.mtd a.out
