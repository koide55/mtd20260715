# Makefile -- System-call-number MTD exercise (e9patch v1.0.1)
#
#   ./setup.sh      # once: fetch & build e9patch v1.0.1 into ./e9patch
#   make            # build tracer + demo programs + instrument hello/evil
#   ./run_demo.sh   # run the full demonstration
#
# e9patch is x86_64-only, so this exercise must run on an x86_64 Linux host.

E9      ?= ./e9patch
CC      ?= gcc

# Instrumentation-target flags: static + non-PIE keeps the layout simple and
# avoids dynamic-loader syscalls confusing the tracer.
TGTFLAGS = -static -no-pie -O2 -Wall

.PHONY: all clean check-e9
all: mtd_tracer hello evil hello.mtd evil.mtd

# --- ptrace supervisor -----------------------------------------------------
mtd_tracer: mtd_tracer.c
	$(CC) -O2 -Wall -o $@ $<

# --- demo programs ---------------------------------------------------------
hello: hello.c
	$(CC) $(TGTFLAGS) -o $@ $<

evil: evil.c
	$(CC) $(TGTFLAGS) -o $@ $<

# Deliberately vulnerable (advanced/authentic demo; see README).
smashme: smashme.c
	$(CC) -fno-stack-protector -z execstack -no-pie -std=c99 -static -o $@ $<

# --- e9patch instrumentation ----------------------------------------------
check-e9:
	@test -x "$(E9)/e9compile.sh" || { \
	  echo "ERROR: e9patch not found in $(E9). Run ./setup.sh first."; exit 1; }

# Compile the hook against e9patch's own stdlib.c (found via -I .../examples).
syscall_mtd: syscall_mtd.c check-e9
	$(E9)/e9compile.sh syscall_mtd.c -I $(E9)/examples

# Insert the hook before every `syscall` instruction of the target.
hello.mtd: hello syscall_mtd check-e9
	$(E9)/e9tool -M 'asm=/syscall/' -P 'before entry(state)@syscall_mtd' hello -o $@

evil.mtd: evil syscall_mtd check-e9
	$(E9)/e9tool -M 'asm=/syscall/' -P 'before entry(state)@syscall_mtd' evil -o $@

smashme.mtd: smashme syscall_mtd check-e9
	$(E9)/e9tool -M 'asm=/syscall/' -P 'before entry(state)@syscall_mtd' smashme -o $@

clean:
	rm -f mtd_tracer hello evil smashme syscall_mtd syscall_mtd.o \
	      hello.mtd evil.mtd smashme.mtd a.out
