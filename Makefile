PREFIX?=x86_64-w64-mingw32

CC=$(PREFIX)-gcc
CXX=$(PREFIX)-g++
AR=$(PREFIX)-ar
LD=$(PREFIX)-ld
OBJCOPY=$(PREFIX)-objcopy
STRIP=$(PREFIX)-strip

GCC_MAJOR := $(shell $(CC) -dumpversion | sed 's/[^0-9].*//')

INCLUDES=build include/winddk include
DEFINES=VK_USE_PLATFORM_WIN32_KHR
LIBS=version gdi32
COMMON_FLAGS=-O2 -fpic -ffunction-sections -fdata-sections -g -MMD -MP -flto -ffile-prefix-map=$(PWD)=./
CXXFLAGS=-std=gnu++17
LDFLAGS=-g -static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic -Wl,--gc-sections
COMMON_FLAGS+=$(addprefix -I,$(INCLUDES)) $(addprefix -D,$(DEFINES))
LDFLAGS+=$(addprefix -l,$(LIBS))

ifeq ($(shell test $(GCC_MAJOR) -ge 14 && echo yes),yes)
    CFLAGS += -std=gnu23
else ifeq ($(shell test $(GCC_MAJOR) -ge 13 && echo yes),yes)
    CFLAGS += -std=gnu2x
endif


#CXXFLAGS+=-pg -no-pie
#COMMON_FLAGS+=-pg -no-pie
#LDFLAGS+=-pg -Wl,--disable-dynamicbase

# Stolen from https://stackoverflow.com/questions/2483182/recursive-wildcards-in-gnu-make/18258352#18258352
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

DEPS := $(call rwildcard,build,*.d)
ifneq ($(DEPS),)
include $(DEPS)
endif

include thirdparty/Makefile.dxvk
include thirdparty/Makefile.triton

UMD_NAME := dx11um_virtio

.DEFAULT_GOAL := build/dist/$(UMD_NAME).dll

.PHONY: format
format:
	clang-format -i *.cpp

.PHONY: clean
clean:
	rm -rf build

DXVK_LIBS=build/dxvk-d3d11.a build/dxvk-dxgi.a build/dxvk.a

UMD_DLL_OBJS=$(addprefix build/,adapter.o device.o dxgi.o resource.o dxvk.o)

build/$(UMD_NAME).dll: $(UMD_DLL_OBJS) $(UMD_NAME).def $(DXVK_LIBS) build/triton.a | build
	$(CXX) $(COMMON_FLAGS) -shared -o $@ $(UMD_DLL_OBJS) $(DXVK_LIBS) build/triton.a -Wl,$(UMD_NAME).def $(LDFLAGS)

build/dist/$(UMD_NAME).dll build/dist/$(UMD_NAME).debug: build/$(UMD_NAME).dll | build/dist
	$(OBJCOPY) --only-keep-debug $< build/dist/$(UMD_NAME).debug
	cp build/$(UMD_NAME).dll build/dist/$(UMD_NAME).dll
	$(STRIP) --strip-debug --strip-unneeded build/dist/$(UMD_NAME).dll
	$(OBJCOPY) --add-gnu-debuglink=build/dist/$(UMD_NAME).debug build/dist/$(UMD_NAME).dll

build/%.o: %.c | build
	$(CC) $(COMMON_FLAGS) $(CFLAGS) $< -c -o $@

build/%.o: %.cpp | build
	$(CC) $(COMMON_FLAGS) $(CXXFLAGS) $< -c -o $@

build:
	mkdir -pv $@

build/dist: | build
	mkdir -pv $@
