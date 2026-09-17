################################################################################
#  EZ8 C/C++ Library                                                           #
################################################################################

TITLE := EZ8 C/C++ Library

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0
VERSION_PRE   :=
VERSION_META  :=

ifneq ($(VERSION_PRE),)
    _VERSION_PRE := -$(VERSION_PRE)
endif

ifneq ($(VERSION_META),)
    _VERSION_META := +$(VERSION_META)
endif

VERSION_BASE := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
VERSION      := $(VERSION_BASE)$(_VERSION_PRE)$(_VERSION_META)

.PHONY: version
version:
	$(info $(VERSION))
	-@cd .

#------------------------------------------------------------------------------#
#  Util                                                                        #
#------------------------------------------------------------------------------#

ifeq ($(OS),Windows_NT)
    EXE_SUFFIX := _win
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        EXE_SUFFIX := $(EXE_SUFFIX)_amd64
    else ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        EXE_SUFFIX := $(EXE_SUFFIX)_amd64
    else ifeq ($(PROCESSOR_ARCHITECTURE),x86)
        EXE_SUFFIX := $(EXE_SUFFIX)_ia32
    endif
    EXE_SUFFIX := $(EXE_SUFFIX).exe
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        EXE_SUFFIX := _linux
    else ifeq ($(UNAME_S),Darwin)
        EXE_SUFFIX := _macos
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        EXE_SUFFIX := $(EXE_SUFFIX)_amd64
    else ifneq ($(filter %86,$(UNAME_P)),)
        EXE_SUFFIX := $(EXE_SUFFIX)_ia32
    else ifneq ($(filter arm%,$(UNAME_P)),)
        EXE_SUFFIX := $(EXE_SUFFIX)_arm
    endif
endif

ifeq ($(shell uname 2>nul),) # Use Command Prompt syntax (Windows)
    MKDIR  = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
    RMDIR  = rmdir /s /q $(subst /,\,$(1)) 1>nul 2>nul || rem
    RMFILE = del /s /q $(subst /,\,$(1)) 1>nul 2>nul
else # Use Bash syntax
    $(shell rm -f nul 2>/dev/null) # Delete file `nul` created by `uname 2>nul`
    MKDIR  = mkdir -p $(1)
    RMDIR  = rm -rf $(1)
    RMFILE = rm -rf $(1)
endif

FONT_RESET      := [0m
FONT_BOLD_GREEN := [1;32m

MAKEFILE := Makefile # Name of this makefile

E :=#       Useful for inserting whitespace where literal whitespace would not be possible
S := $E $E# Useful for inserting a space character where a literal space would not be possible
C := ,#     Useful for inserting a comma character where a literal comma would not be possible
define N #  Useful for inserting a new line where a literal line break would not be possible


endef

#------------------------------------------------------------------------------#
#  Help                                                                        #
#------------------------------------------------------------------------------#

.DEFAULT_GOAL := help
.PHONY: help
help:
	$(info $NUsage: make [TARGET]... [VARIABLE=VALUE]...)
	$(info $NTargets:$N)
	$(info $E   help      Print this help menu)
	$(info $E   version   Print only the project's full version)
	$(info $E   all       Compile library sources and generate the static library archive)
	$(info $E   test      Compile test sources and run the resulting executable (see variable TEST))
	$(info $E   format    Format library and test sources with clang-format)
	$(info $E   lint      Analyze library and test sources with cppcheck)
	$(info $E   clean     Delete all files and directories generated during compilation)
	$(info $NVariables:$N)
	$(info $E   DEBUG              Indicate if debug messages will be compiled (default: 0))
	$(info $E                         DEBUG=1: Include debug messages in the compilation)
	$(info $E   TEST               Select the test to be run (default: assembler))
	$(info $E                         TEST=assembler: Execute the assembler to test an assembly program (see variable TEST_ASM_NUM))
	$(info $E   TEST_ASM_NUM       Define the number of the test assembly program to be parsed (default: 1))
	$(info $E   TOOLCHAIN_PREFIX   Define a prefix for the toolchain commands gcc, ar, ... (default: empty))
	-@cd .

#------------------------------------------------------------------------------#
#  Banner                                                                      #
#------------------------------------------------------------------------------#

define BANNER
$(FONT_BOLD_GREEN)--------------------------------------------------------------------------------
 $(TITLE) $(VERSION)
--------------------------------------------------------------------------------$(FONT_RESET)
endef

ifeq ($(filter version,$(MAKECMDGOALS)),)
    $(info $(BANNER))
endif

#------------------------------------------------------------------------------#
#  Build                                                                       #
#------------------------------------------------------------------------------#

SRC_DIR   := src
INC_DIRS  := $(SRC_DIR)
BUILD_DIR := build
TEST_DIR  := tests
OBJ_DIR   := $(BUILD_DIR)/obj
EXE_DIR   := $(BUILD_DIR)/exe
LIB_DIR   := $(BUILD_DIR)/lib

LIB_FILE  := $(LIB_DIR)/libez8_$(VERSION).a
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SRC_FILES)))
DEP_FILES := $(patsubst %.o,%.d,$(OBJ_FILES))

# test1.asm  | Success
# test2.asm  | Error: More than 1 token in a directive line
# test3.asm  | Error: No END directive
# test4.asm  | Error: Missing operand
# test5.asm  | Error: Unnecessary operand
# test6.asm  | Error: Too many tokens in the same line
# test7.asm  | Error: More than 3 tokens in a code section line
# test8.asm  | Error: Invalid label definition
# test9.asm  | Error: Invalid mnemonic
# test10.asm | Error: Invalid integer operand
# test11.asm | Error: Invalid integer data token
# test12.asm | Error: Invalid string data token
# test13.asm | Error: Undefined label
# test14.asm | Error: Exceeded the addressable memory size
# test15.asm | Error: Exceeded the maximum number of labels
# test16.asm | Error: Label too long

TEST := assembler

ifeq ($(TEST),assembler)
    TEST_ASM_NUM   := 1# Number from the assembly file
    TEST_ASM_DIR   := $(TEST_DIR)/assembler
    TEST_ASM_PROG  := $(TEST_ASM_DIR)/test$(TEST_ASM_NUM).asm
    TEST_SRC_FILES := $(wildcard $(TEST_ASM_DIR)/*.c)
    INC_DIRS       += $(TEST_ASM_DIR)
    EXE_DIR        := $(EXE_DIR)/$(TEST_ASM_DIR)
    EXE_FILE       := $(EXE_DIR)/test_assembler$(EXE_SUFFIX)
    RUN_ARGS       := $(TEST_ASM_PROG)
endif

TEST_OBJ_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(TEST_SRC_FILES)))
TEST_DEP_FILES := $(patsubst %.o,%.d,$(TEST_OBJ_FILES))

VPATH := $(sort $(dir $(SRC_FILES) $(TEST_SRC_FILES)))

CC   := $(TOOLCHAIN_PREFIX)gcc
AR   := $(TOOLCHAIN_PREFIX)ar
SIZE := $(TOOLCHAIN_PREFIX)size

CPP_FLAGS = \
$(addprefix -I ,$(INC_DIRS)) \
-MMD \
-MP \
-MF $(patsubst %.o,%.d,$@) \
-MT $@ \
-D EZ8_VERSION_MAJOR=$(VERSION_MAJOR) \
-D EZ8_VERSION_MINOR=$(VERSION_MINOR) \
-D EZ8_VERSION_PATCH=$(VERSION_PATCH)

ifneq ($(VERSION_PRE),)
    CPP_FLAGS += -D EZ8_VERSION_PRE=\"$(VERSION_PRE)\"
endif

ifneq ($(VERSION_META),)
    CPP_FLAGS += -D EZ8_VERSION_META=\"$(VERSION_META)\"
endif

DEBUG := 0

ifeq ($(DEBUG),1)
    CPP_FLAGS += -D DEBUG
endif

C_FLAGS := \
-c \
-O2 \
-std=c99 \
-Wall \
-Wextra \
-Wpedantic \
-Wshadow \
-Wfloat-equal \
-Wdouble-promotion \
-Wundef \
-Wstrict-prototypes \
-Wswitch-default \
-Wunreachable-code \
-Wwrite-strings \
-Wformat=2 \
-Wconversion \
-Wcast-align \
-Wpointer-arith \
-fno-common \
-save-temps \
-fverbose-asm \
-fstack-usage \
-ffunction-sections \
-fdata-sections \
-Werror

LD_FLAGS := \
-Wl,--gc-sections \
-L $(LIB_DIR) \
-l:$(notdir $(LIB_FILE))

.PHONY: all
all: $(LIB_FILE)

$(LIB_FILE): $(OBJ_FILES) $(MAKEFILE) | $(LIB_DIR)
	$(info $N$(FONT_BOLD_GREEN)Creating static library: $@$(FONT_RESET))
	$(AR) -rcs $@ $(OBJ_FILES)

$(OBJ_FILES): $(OBJ_DIR)/%.o: %.c $(OBJ_DIR)/%.d $(MAKEFILE) | $(OBJ_DIR)
	$(info $N$(FONT_BOLD_GREEN)Compiling: $@$(FONT_RESET))
	$(CC) $(CPP_FLAGS) $(C_FLAGS) -o $@ $<

.PHONY: test
test: $(EXE_FILE)
	$(info $N$(FONT_BOLD_GREEN)Running: $<$(FONT_RESET))
	$(EXE_FILE) $(RUN_ARGS)

$(EXE_FILE): $(LIB_FILE) $(TEST_OBJ_FILES) $(MAKEFILE) | $(EXE_DIR)
	$(info $N$(FONT_BOLD_GREEN)Linking: $@$(FONT_RESET))
	$(CC) -o $@ $(TEST_OBJ_FILES) $(LD_FLAGS)
	$(SIZE) $@

$(TEST_OBJ_FILES): $(OBJ_DIR)/%.o: %.c $(OBJ_DIR)/%.d $(MAKEFILE) | $(OBJ_DIR)
	$(info $N$(FONT_BOLD_GREEN)Compiling: $@$(FONT_RESET))
	$(CC) $(CPP_FLAGS) $(C_FLAGS) -o $@ $<

$(EXE_DIR) $(OBJ_DIR) $(LIB_DIR):
	$(info $N$(FONT_BOLD_GREEN)Creating directory: $@$(FONT_RESET))
	$(call MKDIR,$@)

$(DEP_FILES) $(TEST_DEP_FILES):

-include $(wildcard $(DEP_FILES) $(TEST_DEP_FILES))

#------------------------------------------------------------------------------#
#  Formatting                                                                  #
#------------------------------------------------------------------------------#

FORMAT_DIRS  := $(SRC_DIR) $(TEST_ASM_DIR)
FORMAT_FILES := $(wildcard $(addsuffix /*.c,$(FORMAT_DIRS)) $(addsuffix /*.h,$(FORMAT_DIRS)))

.PHONY: format
format:
	$(info $N$(FONT_BOLD_GREEN)Formatting sources$(FONT_RESET))
	clang-format -i $(FORMAT_FILES)

#------------------------------------------------------------------------------#
#  Code Analysis                                                               #
#------------------------------------------------------------------------------#

LINT_DIRS  := $(SRC_DIR) $(TEST_ASM_DIR)
LINT_FLAGS := \
--platform=native \
--std=c99 \
--check-level=exhaustive \
--enable=all \
--suppress=missingIncludeSystem \
--suppress=unusedFunction \
--showtime=file-total \
$(addprefix -I ,$(INC_DIRS))

.PHONY: lint
lint:
	$(info $N$(FONT_BOLD_GREEN)Analyzing sources$(FONT_RESET))
	cppcheck $(LINT_FLAGS) $(LINT_DIRS)

#------------------------------------------------------------------------------#
#  Clean                                                                       #
#------------------------------------------------------------------------------#

.PHONY: clean
clean:
	$(info $N$(FONT_BOLD_GREEN)Cleaning directory$(FONT_RESET))
	$(call RMDIR,$(BUILD_DIR))

################################# END OF FILE ##################################
