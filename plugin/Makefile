PLUGIN := p2mppatch.so
SDK ?= $(realpath ../sdk)

all: $(PLUGIN)

SRC := plugin.cpp byteseq.cpp module.cpp anchor.cpp suftree.cpp patch.cpp
SRC := $(addprefix src/, $(SRC))
OBJ := $(addprefix obj/, $(SRC:.cpp=.o))

SDKOBJ := memoverride.o
SDKOBJ := $(addprefix obj/sdk/, $(SDKOBJ))
SDKDYNLIB := tier0
SDKSTATLIB := tier1.a mathlib.a tier2.a
SDKSTATLIB := $(addprefix $(SDK)/mp/src/lib/public/linux32/, $(SDKSTATLIB))

CPPFLAGS := -O0 -g3 -m32 -std=gnu++0x -march=core2 -mtune=core2 -msse2 -mfpmath=sse -fpermissive -DNO_HOOK_MALLOC -DNO_MALLOC_OVERRIDE -DGNUC -DCOMPILER_GCC -DPOSIX -D_POSIX -DLINUX -D_LINUX -DDLLNAME=p2mppatch
CFLAGAS := $(CPPFLAGS) -I"src" -I"$(SDK)/mp/src/public" -I"$(SDK)/mp/src/public/tier0" -I"$(SDK)/mp/src/public/tier1" -I"$(COMPILER)/include/c++/4.6.3"
LFLAGS := $(CPPFLAGS) -shared -Wl,--no-undefined -L"$(SDK)/mp/src/lib/public/linux32" -u _CreateInterface -lm -ldl -lpthread

-include $(OBJ:.o=.d)

obj/src/%.o: src/%.cpp
	@mkdir -p $$(dirname "$@")
	$(CXX) $(CFLAGAS) -c -MMD -o "$@" "$<"

obj/sdk/memoverride.o: $(SDK)/mp/src/public/tier0/memoverride.cpp
	@mkdir -p $$(dirname "$@")
	$(CXX) $(CFLAGAS) -c -o "$@" "$<"

$(PLUGIN): $(OBJ) $(SDKOBJ)
	$(CXX) $(LFLAGS) -o "$@" $^ $(addprefix -l,$(SDKDYNLIB)) $(SDKSTATLIB)

.PHONY: clean
clean:
	rm -rf obj $(PLUGIN)