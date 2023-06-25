PLUGIN := p2mppatch.so
SDK ?= $(realpath ../sdk)
STEAM_RUNTIME_PATH ?= $(realpath ../runtime)

all: $(PLUGIN)

SRC := compat.cpp plugin.cpp byteseq.cpp module.cpp anchor.cpp sufarr.cpp scratchpad.cpp patch.cpp detour.cpp hook.cpp patches/anchors.cpp patches/transitions_fix.cpp patches/player_spawn.cpp patches/player_stuck.cpp patches/door_stuck.cpp
SRC := $(addprefix src/, $(SRC))
OBJ := $(addprefix obj/, $(SRC:.cpp=.o))

SDKOBJ := memoverride.o
SDKOBJ := $(addprefix obj/sdk/, $(SDKOBJ))
SDKDYNLIB := tier0 vstdlib
SDKSTATLIB := tier1.a mathlib.a tier2.a
SDKSTATLIB := $(addprefix $(SDK)/mp/src/lib/public/linux32/, $(SDKSTATLIB))

# OPTFLAGS ?= -O0 -g3
# OPTFLAGS ?= -O0 -g3 -fsanitize=address
OPTFLAGS ?= -O3 -g3

ifeq ($(OS), Windows_NT)
	OSFLAGS := -DWIN32 -D_WIN32
else
	OSFLAGS := -DPOSIX -D_POSIX -DLINUX -D_LINUX 
endif

CPPFLAGS := $(OPTFLAGS) -m32 -std=gnu++17 -march=core2 -mtune=core2 -msse2 -mfpmath=sse -fpermissive -DNO_HOOK_MALLOC -DNO_MALLOC_OVERRIDE -DGNUC -DCOMPILER_GCC -DDLLNAME=p2mppatch
CFLAGS := $(CPPFLAGS) -I"src" -I"$(SDK)/mp/src/public" -I"$(SDK)/mp/src/public/tier0" -I"$(SDK)/mp/src/public/tier1" -I"$(COMPILER)/include/c++/4.6.3"
LFLAGS := $(CPPFLAGS) -shared -Wl,--no-undefined -L"$(SDK)/mp/src/lib/public/linux32" -u _CreateInterface -lm -ldl -lpthread

-include $(OBJ:.o=.d)

obj/src/%.o: src/%.cpp
	@mkdir -p $$(dirname "$@")
	$(CXX) $(CFLAGS) -c -MMD -o "$@" "$<"

obj/sdk/memoverride.o: $(SDK)/mp/src/public/tier0/memoverride.cpp
	@mkdir -p $$(dirname "$@")
	$(CXX) $(CFLAGS) -c -o "$@" "$<"

$(PLUGIN): $(OBJ) $(SDKOBJ)
	$(CXX) $(LFLAGS) -o "$@" $^ $(SDKSTATLIB) $(addprefix -l,$(SDKDYNLIB))

.PHONY: clean
clean:
	rm -rf obj $(PLUGIN)