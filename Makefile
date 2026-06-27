TARGET := build/version.dll
HELPER_TARGET := build/cgss-borderless-helper.exe
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
PACKAGE_DIR := $(BUILD_DIR)/package
PACKAGE_NAME := cgss-dmm-hook
PACKAGE_ROOT := $(PACKAGE_DIR)/$(PACKAGE_NAME)
BUILD ?= release

CC := x86_64-w64-mingw32-gcc
CXX := x86_64-w64-mingw32-g++
WINDRES := x86_64-w64-mingw32-windres
WINPTHREAD_DLL := /usr/x86_64-w64-mingw32/bin/libwinpthread-1.dll

INCLUDES := -Isrc -Ideps/minhook/include -Ideps/minhook/src -Ideps/minhook/src/hde
INCLUDES += -Ideps/rapidjson/include
DEFINES := -DNOMINMAX -DWIN32_LEAN_AND_MEAN
CFLAGS := -std=gnu11 $(DEFINES) $(INCLUDES)
CXXFLAGS := -std=gnu++20 -fno-exceptions -fno-rtti $(DEFINES) $(INCLUDES)
LDFLAGS := -shared -static-libgcc -static-libstdc++
LDLIBS := -luser32 -lkernel32
HELPER_LDFLAGS := -static-libgcc -static-libstdc++ -mwindows
HELPER_LDLIBS := -luser32 -lkernel32 -lshell32

ifeq ($(BUILD),debug)
  DEFINES += -D_DEBUG
  CFLAGS += -O0 -g
  CXXFLAGS += -O0 -g
else
  CFLAGS += -O2 -ffunction-sections -fdata-sections
  CXXFLAGS += -O2 -ffunction-sections -fdata-sections
  LDFLAGS += -Wl,--gc-sections -s
  HELPER_LDFLAGS += -Wl,--gc-sections -s
endif

C_SRCS := \
	deps/minhook/src/buffer.c \
	deps/minhook/src/hook.c \
	deps/minhook/src/trampoline.c \
	deps/minhook/src/hde/hde64.c

CXX_SRCS := \
	src/proxy.cpp \
	src/hook.cpp \
	src/log.cpp \
	src/config.cpp \
	src/il2cpp_symbols.cpp

HELPER_CXX_SRCS := \
	src/helper_main.cpp

RC_SRCS := src/version.rc
HELPER_RC_SRCS := src/helper_version.rc
DEF_FILE := src/version.def

C_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SRCS))
CXX_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CXX_SRCS))
HELPER_CXX_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(HELPER_CXX_SRCS))
RC_OBJS := $(patsubst %.rc,$(OBJ_DIR)/%.res,$(RC_SRCS))
HELPER_RC_OBJS := $(patsubst %.rc,$(OBJ_DIR)/%.res,$(HELPER_RC_SRCS))
OBJS := $(C_OBJS) $(CXX_OBJS) $(RC_OBJS)

.PHONY: all release debug package clean

all: $(TARGET) $(HELPER_TARGET) $(BUILD_DIR)/libwinpthread-1.dll
release: $(TARGET) $(HELPER_TARGET) $(BUILD_DIR)/libwinpthread-1.dll
debug:
	$(MAKE) BUILD=debug
package: $(PACKAGE_DIR)/$(PACKAGE_NAME).zip

$(TARGET): $(OBJS) $(DEF_FILE)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(DEF_FILE) $(LDLIBS)

$(HELPER_TARGET): $(HELPER_CXX_OBJS) $(HELPER_RC_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(HELPER_LDFLAGS) -o $@ $(HELPER_CXX_OBJS) $(HELPER_RC_OBJS) $(HELPER_LDLIBS)

$(BUILD_DIR)/libwinpthread-1.dll:
	@mkdir -p $(dir $@)
	cp "$(WINPTHREAD_DLL)" "$@"

$(PACKAGE_DIR)/$(PACKAGE_NAME).zip: $(TARGET) $(HELPER_TARGET) $(BUILD_DIR)/libwinpthread-1.dll config.json.example README.md LICENSE THIRD_PARTY_NOTICES.md
	rm -rf "$(PACKAGE_ROOT)"
	@mkdir -p "$(PACKAGE_ROOT)"
	cp "$(TARGET)" "$(PACKAGE_ROOT)/version.dll"
	cp "$(HELPER_TARGET)" "$(PACKAGE_ROOT)/cgss-borderless-helper.exe"
	cp "$(BUILD_DIR)/libwinpthread-1.dll" "$(PACKAGE_ROOT)/libwinpthread-1.dll"
	cp config.json.example "$(PACKAGE_ROOT)/config.json.example"
	cp README.md LICENSE THIRD_PARTY_NOTICES.md "$(PACKAGE_ROOT)/"
	cd "$(PACKAGE_DIR)" && zip -qr "$(PACKAGE_NAME).zip" "$(PACKAGE_NAME)"

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.res: %.rc
	@mkdir -p $(dir $@)
	$(WINDRES) $(DEFINES) $(INCLUDES) $< -O coff -o $@

clean:
	rm -rf $(BUILD_DIR)
