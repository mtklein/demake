# Host-native test build: the REAL game logic (src/menu.c, data.c, party5.c,
# encounter.c, game.c + rules/ + generated tables) linked against a fake
# engine and run under ASan/UBSan, with an llvm-cov coverage mode.
#
# Standalone by design -- it does not touch the top-level Makefile.
#   make -C test/host sim              run the suite under sanitizers
#   make -C test/host sim ARGS=fuzz    run tests matching a substring
#   make -C test/host cov              rebuild instrumented + coverage table

ROOT := $(abspath ../..)
GEN  := $(ROOT)/build/gen
OUT  := $(ROOT)/build/host
LLVM := /opt/homebrew/opt/llvm/bin

GAME  := $(ROOT)/src/menu.c $(ROOT)/src/data.c $(ROOT)/src/party5.c \
         $(ROOT)/src/encounter.c $(ROOT)/src/game.c
RULES := $(ROOT)/rules/dice.c $(ROOT)/rules/core.c $(ROOT)/rules/features.c
GENC  := $(GEN)/srd_tables.c $(GEN)/assets.c $(GEN)/screens.c
HOST  := fake_engine.c sim_main.c
SRCS  := $(GAME) $(RULES) $(GENC) $(HOST)

# shim/gba.h must beat src/gba.h. Quoted includes inside src/*.c always
# search src/ first regardless of -I order, so the shim is force-included
# (-include) and claims the real header's guard; the -I order covers
# everyone else (generated + rules sources).
INC    := -Ishim -I$(ROOT)/src -I$(ROOT)/rules -I$(GEN) -I.
CFLAGS := -std=c11 -g -O1 -fno-omit-frame-pointer -Wall -Wextra \
          -include shim/gba.h $(INC)
SAN    := -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: sim cov gen clean

sim: $(OUT)/sim
	$(OUT)/sim $(ARGS)

$(OUT)/sim: $(SRCS) shim/gba.h fake_engine.h Makefile | $(OUT)
	cc $(CFLAGS) $(SAN) -o $@ $(SRCS)

# generated tables come from the same tools the ROM build uses
$(GENC):
	$(MAKE) -C $(ROOT) build/gen/srd_tables.c build/gen/assets.c

gen: $(GENC)

$(OUT):
	mkdir -p $(OUT)

# Coverage: build with Homebrew LLVM so clang / llvm-profdata / llvm-cov
# agree on profile format. Report covers the game sources compiled in.
cov: $(SRCS) shim/gba.h fake_engine.h | $(OUT)
	$(LLVM)/clang $(CFLAGS) -fprofile-instr-generate -fcoverage-mapping \
	    -o $(OUT)/sim_cov $(SRCS)
	LLVM_PROFILE_FILE=$(OUT)/sim.profraw $(OUT)/sim_cov $(ARGS)
	$(LLVM)/llvm-profdata merge -sparse $(OUT)/sim.profraw -o $(OUT)/sim.profdata
	$(LLVM)/llvm-cov report $(OUT)/sim_cov -instr-profile=$(OUT)/sim.profdata \
	    $(GAME) $(RULES)

clean:
	rm -rf $(OUT)
