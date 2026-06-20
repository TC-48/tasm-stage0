CC ?= cc
PY ?= python3
BUILD ?= release

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

ifeq ($(OS),Windows_NT)
	PLATFORM := windows
else
	PLATFORM := posix
endif

SRC_DIR     := src
TEST_DIR    := tests
INCLUDE_DIR := include
DEPS_DIR    := deps

OBJ_ROOT_DIR := build/$(BUILD)/obj
DEP_ROOT_DIR := build/$(BUILD)/dep

OUT_DIR     := out/$(BUILD)
LIB_DIR     := $(OUT_DIR)/lib
BIN_DIR     := $(OUT_DIR)/bin

LIB_NAME    := tasm
LIB_STATIC  := $(LIB_DIR)/lib$(LIB_NAME).a
LIB_SHARED  := $(LIB_DIR)/lib$(LIB_NAME).so

EXE_EXT :=
ifeq ($(PLATFORM),windows)
	EXE_EXT := .exe
else ifeq ($(PLATFORM),posix)
	EXE_EXT :=
endif

TARGET := $(BIN_DIR)/tasm$(EXE_EXT)

CSTD       := -std=c11
WARNINGS   := -Wall -Wextra -Werror=implicit-fallthrough -Wno-old-style-declaration
PIC_CFLAGS := -fPIC

EMU_DIR := $(DEPS_DIR)/tc48-emu
EMU_GEN_HEADERS := include/tc48/gen/word-lits.h \
                   include/tc48/gen/version.h
EMU_LIB_STATIC  := $(EMU_DIR)/out/$(BUILD)/lib/libtc48emu.a

TSCS_SCRIPT     := $(DEPS_DIR)/tscs-spec/scripts/gen-c-table.py
TSCS_SPEC_JSON  := $(DEPS_DIR)/tscs-spec/spec.json
TSCS_GEN_HEADER := $(INCLUDE_DIR)/tasm/gen/tscs.h
TSCS_GEN_SOURCE := $(SRC_DIR)/gen/tscs.c

COMMON_CFLAGS := $(CSTD) $(WARNINGS) -I$(INCLUDE_DIR) -I$(EMU_DIR)/include -I$(DEPS_DIR)/strlib/src

ifeq ($(BUILD),debug)
	CFLAGS := $(COMMON_CFLAGS) -O0 -g -DTC48_DEBUG -fsanitize=address,undefined
	LDFLAGS := -fsanitize=address,undefined
else ifeq ($(BUILD),release)
	CFLAGS := $(COMMON_CFLAGS) -O3 -DNDEBUG
	LDFLAGS := -flto
else
	$(error Unknown BUILD=$(BUILD))
endif

ifeq ($(PLATFORM),windows)
	CMD_MKDIR_P = powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\,$(1))'"
	CMD_RM_RF   = powershell -NoProfile -Command "Remove-Item -Recurse -Force -Path '$(subst /,\,$(1))'"
else
	CMD_MKDIR_P = mkdir -p "$(1)"
	CMD_RM_RF   = rm -rf "$(1)"
endif

ALL_C_SRCS := $(call rwildcard,$(SRC_DIR),*.c)
ALL_C_SRCS := $(sort $(ALL_C_SRCS) $(TSCS_GEN_SOURCE))

MAIN_C_SRC := $(SRC_DIR)/main.c
LIB_C_SRCS := $(filter $(SRC_DIR)/%, $(filter-out $(MAIN_C_SRC), $(ALL_C_SRCS)))

MAIN_OBJ := $(patsubst %.c,$(OBJ_ROOT_DIR)/%.o,$(MAIN_C_SRC))
LIB_OBJ_STATIC := $(patsubst %.c,$(OBJ_ROOT_DIR)/%.o,$(LIB_C_SRCS))
LIB_OBJ_SHARED := $(patsubst %.c,$(OBJ_ROOT_DIR)/shared/%.o,$(LIB_C_SRCS))

DEPS := $(patsubst %.c,$(DEP_ROOT_DIR)/%.d,$(ALL_C_SRCS)) \
        $(patsubst %.c,$(DEP_ROOT_DIR)/shared/%.d,$(LIB_C_SRCS))

JFLAG := $(filter -j%,$(MAKEFLAGS))
ifeq ($(JFLAG),)
	JFLAG := -j1
endif

.PHONY: all dirs clean sharedlib

all: $(TARGET) $(LIB_STATIC) $(LIB_SHARED)

$(EMU_GEN_HEADERS):
	$(MAKE) -C $(EMU_DIR) FEATURES=none $@

$(TSCS_GEN_HEADER) $(TSCS_GEN_SOURCE) &: $(TSCS_SCRIPT)
	@$(call CMD_MKDIR_P,$(dir $(TSCS_GEN_HEADER)))
	@$(call CMD_MKDIR_P,$(dir $(TSCS_GEN_SOURCE)))
	$(PY) $(TSCS_SCRIPT) $(TSCS_SPEC_JSON) --inc-path="<tasm/gen/tscs.h>" --out-c=$(TSCS_GEN_SOURCE) --out-h=$(TSCS_GEN_HEADER)

$(EMU_LIB_STATIC): $(EMU_GEN_HEADERS)
	$(MAKE) -C $(EMU_DIR) libtc48emu FEATURES=none BUILD=$(BUILD)

$(LIB_STATIC): $(LIB_OBJ_STATIC)
	@$(call CMD_MKDIR_P,$(dir $@))
	ar rcs $@ $^

$(LIB_SHARED): $(LIB_OBJ_SHARED)
	@$(call CMD_MKDIR_P,$(dir $@))
	$(CC) -shared $^ $(LDFLAGS) -o $@

$(TARGET): $(LIB_STATIC) $(MAIN_OBJ) $(EMU_LIB_STATIC)
	@$(call CMD_MKDIR_P,$(dir $@))
	$(CC) $(MAIN_OBJ) $(LIB_STATIC) $(EMU_LIB_STATIC) $(LDFLAGS) -o $@

$(OBJ_ROOT_DIR)/%.o: %.c $(EMU_GEN_HEADERS)
	@$(call CMD_MKDIR_P,$(dir $@))
	@$(call CMD_MKDIR_P,$(DEP_ROOT_DIR)/$(dir $<))
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_ROOT_DIR)/$*.d -c $< -o $@

$(OBJ_ROOT_DIR)/shared/%.o: %.c $(EMU_GEN_HEADERS)
	@$(call CMD_MKDIR_P,$(dir $@))
	@$(call CMD_MKDIR_P,$(DEP_ROOT_DIR)/shared/$(dir $<))
	$(CC) $(CFLAGS) $(PIC_CFLAGS) -MMD -MP -MF $(DEP_ROOT_DIR)/shared/$*.d -c $< -o $@

sharedlib: $(LIB_SHARED)

-include $(DEPS)
include e2e-tests/run.mk

clean: e2e-clean
	@$(call CMD_RM_RF,build)
	@$(call CMD_RM_RF,out)

