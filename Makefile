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
           -Isrc -I$(BUILD)/gen
LDFLAGS := $(ARCH) -nostdlib -T gba.ld -Wl,-Map,$(BUILD)/rom.map

CSRC   := $(wildcard src/*.c)
GENSRC := $(BUILD)/gen/assets.c
OBJ    := $(BUILD)/crt0.o \
          $(patsubst src/%.c,$(BUILD)/%.o,$(CSRC)) \
          $(BUILD)/gen/assets.o

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

$(BUILD)/%.o: src/%.c src/gba.h $(BUILD)/gen/assets.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/gen/assets.o: $(BUILD)/gen/assets.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/gen/assets.c $(BUILD)/gen/assets.h: $(TOOLSRC) | $(BUILD)
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
