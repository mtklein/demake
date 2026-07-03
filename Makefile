PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
PY      := python3

BUILD := build
ROM   := $(BUILD)/nautiloid.gba
ELF   := $(BUILD)/nautiloid.elf
TITLE := NAUTILOID

ARCH    := -mcpu=arm7tdmi -mthumb
CFLAGS  := $(ARCH) -O2 -g -Wall -Wextra -ffreestanding -fno-strict-aliasing \
           -Isrc -Irules -I$(BUILD)/gen
LDFLAGS := $(ARCH) -nostdlib -T gba.ld -Wl,-Map,$(BUILD)/rom.map
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
	$(CC) $(LDFLAGS) -o $@ $(OBJ) -lgcc

$(BUILD)/crt0.o: src/crt0.s | $(BUILD)
	$(CC) -mcpu=arm7tdmi -x assembler-with-cpp -c $< -o $@

$(BUILD)/%.o: src/%.c src/gba.h $(BUILD)/gen/assets.h $(BUILD)/gen/screens.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/gen/%.o: $(BUILD)/gen/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/r_%.o: rules/%.c rules/rules.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(SRDTAB): tools/mksrd.py tools/srd/srd_data.py tools/srd/overrides.py | $(BUILD)
	$(PY) tools/mksrd.py

.PHONY: test-rules
test-rules: $(BUILD)/test_rules
	$(BUILD)/test_rules

$(BUILD)/test_rules: $(RULESSRC) $(SRDTAB) rules/rules.h rules/test_rules.c | $(BUILD)
	$(HOSTCC) -O1 -g -Wall -Wextra -Werror -Irules \
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
