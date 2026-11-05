CC = clang
CFLAGS = -O2 -Wall

# macOS SDK 경로 자동 탐색 (오류 방지용)
ifeq ($(shell uname), Darwin)
	SDK_PATH = $(shell ls -d /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk 2>/dev/null | head -n 1)
	ifeq ($(SDK_PATH),)
		SDK_PATH = $(shell ls -d /Library/Developer/CommandLineTools/SDKs/MacOSX15*.sdk 2>/dev/null | head -n 1)
	endif
	ifneq ($(SDK_PATH),)
		CFLAGS += -isysroot $(SDK_PATH)
	endif
endif

TARGETS = dungeon_chest auth_core

all: $(TARGETS)

dungeon_chest: dungeon_chest.c
	$(CC) $(CFLAGS) -o dungeon_chest dungeon_chest.c

auth_core: auth_core.c
	$(CC) $(CFLAGS) -o auth_core auth_core.c

clean:
	rm -rf $(TARGETS) *.dSYM

.PHONY: all clean
