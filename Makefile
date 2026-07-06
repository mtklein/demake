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

$(BUILD)/%.o: src/%.c src/gba.h $(BUILD)/gen/assets.h $(BUILD)/gen/screens.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/gen/%.o: $(BUILD)/gen/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/r_%.o: rules/%.c rules/rules.h | $(BUILD)
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

# the whole pre-commit ritual: build, native suites, lint, 7 playthroughs
gate:
	test/gate.sh

$(BUILD)/test_rules: $(RULESSRC) $(SRDTAB) $(BUILD)/gen/srd_ids.h rules/rules.h rules/test_rules.c | $(BUILD)
	$(HOSTCC) -O1 -g -Wall -Wextra -Werror -Irules -I$(BUILD)/gen \
	    -o $@ $(RULESSRC) $(SRDTAB) rules/test_rules.c

$(BUILD)/gen/assets.c $(BUILD)/gen/assets.h $(BUILD)/gen/screens.c $(BUILD)/gen/screens.h: $(TOOLSRC) | $(BUILD)
	$(PY) tools/mkassets.py

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
