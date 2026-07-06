# Cross-compiled with Homebrew LLVM: clang + ld.lld + llvm-objcopy
# (brew install llvm lld). src/aeabi.c supplies the AEABI division helpers
# that libgcc used to provide -- the only runtime the game needs.
LLVM    := $(shell brew --prefix llvm)
LLD     := $(shell brew --prefix lld)
CC      := $(LLVM)/bin/clang
OBJCOPY := $(LLVM)/bin/llvm-objcopy
PY      := python3

BUILD := build
ROM   := $(BUILD)/nautiloid.gba
ELF   := $(BUILD)/nautiloid.elf
TITLE := NAUTILOID

ARCH    := --target=arm-none-eabi -mcpu=arm7tdmi -mthumb
CFLAGS  := $(ARCH) -O2 -g -Wall -Wextra -ffreestanding -fno-strict-aliasing \
           -Isrc -Irules -I$(BUILD)/gen
LDFLAGS := $(ARCH) -nostdlib --ld-path=$(LLD)/bin/ld.lld -T gba.ld \
           -Wl,-Map,$(BUILD)/rom.map
HOSTCC  ?= cc

CSRC     := $(wildcard src/*.c)
RULESSRC := rules/dice.c rules/core.c rules/features.c
SRDTAB   := $(BUILD)/gen/srd_tables.c
GENSRC   := $(BUILD)/gen/assets.c $(BUILD)/gen/screens.c $(SRDTAB)
OBJ      := $(BUILD)/crt0.o \
            $(patsubst src/%.c,$(BUILD)/%.o,$(CSRC)) \
            $(patsubst rules/%.c,$(BUILD)/r_%.o,$(RULESSRC)) \
            $(BUILD)/gen/assets.o $(BUILD)/gen/screens.o $(BUILD)/gen/srd_tables.o

TOOLSRC := $(wildcard tools/*.py tools/art/*.py tools/music/*.py)

.PHONY: all clean run
all: $(ROM)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(PY) tools/fixrom.py $@ $(TITLE)

$(ELF): $(OBJ) gba.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

$(BUILD)/crt0.o: src/crt0.s | $(BUILD)
	$(CC) --target=arm-none-eabi -mcpu=arm7tdmi -x assembler-with-cpp -c $< -o $@

$(BUILD)/%.o: src/%.c src/gba.h $(BUILD)/gen/assets.h $(BUILD)/gen/screens.h $(BUILD)/gen/srd_ids.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/gen/%.o: $(BUILD)/gen/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/r_%.o: rules/%.c rules/rules.h $(BUILD)/gen/srd_ids.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(SRDTAB) $(BUILD)/gen/srd_ids.h: tools/mksrd.py tools/srd/srd_data.py tools/srd/overrides.py | $(BUILD)
	$(PY) tools/mksrd.py

.PHONY: test-rules test-aeabi gate
test-rules: $(BUILD)/test_rules
	$(BUILD)/test_rules

test-aeabi: $(BUILD)/test_aeabi
	$(BUILD)/test_aeabi

$(BUILD)/test_aeabi: src/aeabi.c test/test_aeabi.c | $(BUILD)
	$(HOSTCC) -O2 -fno-builtin -Wall -Wextra -o $@ src/aeabi.c test/test_aeabi.c

# the whole pre-commit ritual: build, native suites, lint, 7 playthroughs,
# then coverage numbers + ratchet (Makefile-level wiring; gate.sh is scenarios)
gate:
	test/gate.sh
	$(MAKE) coverage

$(BUILD)/test_rules: $(RULESSRC) $(SRDTAB) $(BUILD)/gen/srd_ids.h rules/rules.h rules/test_rules.c | $(BUILD)
	$(HOSTCC) -O1 -g -Wall -Wextra -Werror -Irules -I$(BUILD)/gen \
	    -o $@ $(RULESSRC) $(SRDTAB) rules/test_rules.c

# --- rules-engine coverage: hard data, ratcheted (`make coverage`) ---------
# Instrumented host build of the same sources as test_rules. Prints the
# per-file Regions/Functions/Lines/Branches table (scoped to the engine
# sources; the test file and generated SRD tables are compiled in but not
# what we measure), writes browsable HTML to build/cov/html/, and fails
# loudly if TOTAL line coverage drops below COV_FLOOR.
COVDIR := $(BUILD)/cov
# ratchet: raise as coverage rises; never lower
COV_FLOOR := 94

$(COVDIR)/test_rules: $(RULESSRC) $(SRDTAB) $(BUILD)/gen/srd_ids.h rules/rules.h rules/test_rules.c | $(BUILD)
	mkdir -p $(COVDIR)
	$(LLVM)/bin/clang -fprofile-instr-generate -fcoverage-mapping -g -O1 \
	    -Wall -Wextra -Werror -Irules -I$(BUILD)/gen \
	    -o $@ $(RULESSRC) $(SRDTAB) rules/test_rules.c

.PHONY: coverage
coverage: $(COVDIR)/test_rules
	LLVM_PROFILE_FILE=$(COVDIR)/rules.profraw $(COVDIR)/test_rules
	$(LLVM)/bin/llvm-profdata merge -sparse $(COVDIR)/rules.profraw \
	    -o $(COVDIR)/rules.profdata
	$(LLVM)/bin/llvm-cov show $(COVDIR)/test_rules \
	    -instr-profile=$(COVDIR)/rules.profdata \
	    -format=html -output-dir=$(COVDIR)/html $(RULESSRC)
	$(LLVM)/bin/llvm-cov report $(COVDIR)/test_rules \
	    -instr-profile=$(COVDIR)/rules.profdata $(RULESSRC) \
	    > $(COVDIR)/report.txt
	@cat $(COVDIR)/report.txt
	@awk -v floor=$(COV_FLOOR) '/^TOTAL/ { seen = 1; pct = $$(NF-3); sub(/%/, "", pct); \
	        if (pct + 0 < floor + 0) { \
	            printf "COVERAGE RATCHET: lines %s%% < floor %s%%\n", pct, floor; exit 1; } \
	        printf "coverage: lines %s%% >= floor %s%% (ratchet ok)\n", pct, floor; } \
	    END { if (!seen) { print "COVERAGE RATCHET: no TOTAL in llvm-cov report"; exit 1; } }' \
	    $(COVDIR)/report.txt

$(BUILD)/gen/assets.c $(BUILD)/gen/assets.h $(BUILD)/gen/screens.c $(BUILD)/gen/screens.h: $(TOOLSRC) | $(BUILD)
	$(PY) tools/mkassets.py

# build id for the crash screen; only rewritten when it changes
.PHONY: FORCE
$(BUILD)/gen/buildid.h: FORCE | $(BUILD)
	@id=$$(git describe --always --dirty 2>/dev/null || echo dev); \
	 new="#define BUILD_ID \"$$id\""; \
	 [ "$$new" = "$$(cat $@ 2>/dev/null)" ] || echo "$$new" > $@

$(BUILD)/panic.o: $(BUILD)/gen/buildid.h

$(BUILD):
	mkdir -p $(BUILD)/gen

run: $(ROM)
	mgba -3 $(ROM)

MGBA_PREFIX := $(shell brew --prefix mgba)
runner: $(BUILD)/runner
$(BUILD)/runner: test/runner.c | $(BUILD)
	cc -O2 -Wall -I$(MGBA_PREFIX)/include -L$(MGBA_PREFIX)/lib -lmgba \
	   -Wl,-rpath,$(MGBA_PREFIX)/lib -o $@ $<

clean:
	rm -rf $(BUILD)
