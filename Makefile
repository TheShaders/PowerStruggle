### User Configuration ###

# Name of ROM to build
TARGET := powerstruggle

DEBUG ?= 0

# Do not change below this line!

# Set powershell as the shell on Windows
ifeq ($(OS),Windows_NT)
  SHELL := powershell.exe
  .SHELLFLAGS := -NoProfile -Command
endif

# Default config generation
ifeq ($(OS),Windows_NT)
  ifeq (,$(wildcard ./Makefile.winconfig))
    DUMMY != New-Item Makefile.winconfig -ItemType File
    DUMMY != Add-Content Makefile.winconfig "\# Location of your mips gcc (must have a slash after it!)"
    DUMMY != Add-Content Makefile.winconfig "N64CHAIN := C:/n64/gcc-toolchain-mips64-win64/bin/"
    DUMMY != Add-Content Makefile.winconfig "\# Location of your N64 SDK"
    DUMMY != Add-Content Makefile.winconfig "SDK := C:/n64/n64sdk"
    DUMMY != Add-Content Makefile.winconfig "\# Python command, e.g. python3 (or location if not on the path)"
    DUMMY != Add-Content Makefile.winconfig "PYTHON := python"
    DUMMY != Add-Content Makefile.winconfig "\# Location of Everdrive usb64 application"
    DUMMY != Add-Content Makefile.winconfig "UNFLOADER := C:/n64/UNFLoader.exe)"
  endif
else
  ifeq (,$(wildcard Makefile.config))
    DUMMY != printf "\# Python command, e.g. python3 (or location if not on the path)\nPYTHON := python3\n" > Makefile.config
    DUMMY != printf "\# UNFLoader location\nUNFLOADER := UNFLoader" >> Makefile.config
  endif
endif

PLATFORM := n64

# System config
ifeq ($(OS),Windows_NT)
  include Makefile.winconfig
  SDK_INCLUDE := -I$(SDK)/ultra/usr/include
  ULTRA_LINKER := -L $(SDK)/ultra/usr/lib
  GCC_LINKER := -L $(N64CHAIN)../lib/gcc/mips64-elf/10.1.0
  LIBULTRA := gultra_rom
else
  include Makefile.config
  SDK_INCLUDE := -I /etc/n64/usr/include
  ULTRA_LINKER := -L /etc/n64/lib
  GCC_LINKER := -L $(N64_LIBGCCDIR)
  LIBULTRA := ultra_rom
endif

### Text variables ###

# These use the fact that += always adds a space to create a variable that is just a space
# Space has a single space, indent has 2
space :=
space +=

indent =
indent += 
indent += 

### Tools ###

# System tools
CD    := cd
CP    := cp
RM    := rm
CAT   := cat
GPERF := gperf


ifeq ($(OS),Windows_NT)
  MKDIR := New-Item -Path 
  MKDIR_OPTS := -Force -Type Directory | Out-Null
  
  RMDIR := (Remove-Item
  RMDIR_OPTS := -ErrorAction SilentlyContinue -Recurse -Force) -or $$true | Out-Null # The -or $true is needed to prevent make from saying error
  
  RUN := cmd /c
  
  PRINT := Write-Host -NoNewline 
   ENDCOLOR := $(PRINT) "$$([char]27)[0m"; 
   WHITE     := 
   ENDWHITE  := $(space) -ForegroundColor White; $(ENDCOLOR) $(PRINT) 
   GREEN     := 
   ENDGREEN  := $(space) -ForegroundColor Green; $(ENDCOLOR) $(PRINT) 
   BLUE      := 
   ENDBLUE   := $(space) -ForegroundColor Blue; $(ENDCOLOR) $(PRINT) 
   YELLOW    := 
   ENDYELLOW := $(space) -ForegroundColor Yellow; $(ENDCOLOR) $(PRINT) 
  ENDLINE := ; Write-Host 

  PREFIX  := mips64-elf-
else
  MKDIR := mkdir
  MKDIR_OPTS := -p

  RMDIR := rm
  RMDIR_OPTS := -rf

  PRINT := printf '
   ENDCOLOR := \033[0m
   WHITE     := \033[0m
   ENDWHITE  := $(ENDCOLOR)
   GREEN     := \033[0;32m
   ENDGREEN  := $(ENDCOLOR)
   BLUE      := \033[0;34m
   ENDBLUE   := $(ENDCOLOR)
   YELLOW    := \033[0;33m
   ENDYELLOW := $(ENDCOLOR)
  ENDLINE := \n'

  RUN := 
  
  PREFIX  := mips-n64-
endif

# Build tools
CC      := $(N64CHAIN)$(PREFIX)gcc
AS      := $(N64CHAIN)$(PREFIX)as
CPP     := $(N64CHAIN)$(PREFIX)cpp
CXX     := $(N64CHAIN)$(PREFIX)g++
LD      := $(N64CHAIN)$(PREFIX)g++
OBJCOPY := $(N64CHAIN)$(PREFIX)objcopy

CKSUM   := $(PYTHON) tools/n64cksum.py

ASSETPACK := tools/assetpack/assetpack
GLTF64    := tools/gltf64/gltf64
LEVELCONV := tools/levelconv/levelconv

TOOLS := $(ASSETPACK) $(GLTF64) $(LEVELCONV)

### Files and Directories ###

# Source files
PLATFORM_DIR := platforms/$(PLATFORM)
SRC_ROOT  := src
SRC_DIRS  := $(wildcard $(SRC_ROOT)/*) $(wildcard $(PLATFORM_DIR)/$(SRC_ROOT)/*)
C_SRCS    := $(foreach src_dir,$(SRC_DIRS),$(wildcard $(src_dir)/*.c))
CXX_SRCS  := $(foreach src_dir,$(SRC_DIRS),$(wildcard $(src_dir)/*.cpp))
ASM_SRCS  := $(foreach src_dir,$(SRC_DIRS),$(wildcard $(src_dir)/*.s))
BIN_FILES := $(foreach src_dir,$(SRC_DIRS),$(wildcard $(src_dir)/*.bin))
SEG_C_SRCS   := $(wildcard segments/*.c)
SEG_CPP_SRCS := $(wildcard segments/*.cpp)
LD_SCRIPT := n64.ld
BOOT      := $(PLATFORM_DIR)/boot/boot.6102
ENTRY_AS  := $(PLATFORM_DIR)/boot/entry.s

# Build root
ifeq ($(DEBUG),0)
BUILD_ROOT     := build/$(PLATFORM)/release
else
BUILD_ROOT     := build/$(PLATFORM)/debug
endif

# Asset files
ASSET_ROOT := assets

MODEL_DIR  := $(ASSET_ROOT)/models
MODELS     := $(wildcard $(MODEL_DIR)/*.gltf)
MODELS_OUT := $(addprefix $(BUILD_ROOT)/, $(MODELS:.gltf=))

LEVELS_DIR := $(ASSET_ROOT)/levels
LEVELS     := $(wildcard $(LEVELS_DIR)/*.level)
LEVELS_OUT := $(addprefix $(BUILD_ROOT)/, $(LEVELS:.level=))

ASSETS_OUT  := $(MODELS_OUT) $(LEVELS_OUT)

LEVEL_DIR := $(ASSET_ROOT)/levels
LEVELS    := $(wildcard $(MODEL_DIR)/*.json)

ASSETS_DIRS  := $(LEVEL_DIR) $(MODEL_DIR)
ASSETS_BIN   := $(BUILD_ROOT)/assets.bin
ASSETS_OBJ   := $(ASSETS_BIN:.bin=.o)
# gperf input file
ASSETS_TXT   := $(ASSETS_BIN:.bin=.txt)
ASSETS_GPERF := $(ASSETS_BIN:.bin=_gperf.txt)
ASSETS_CPP   := $(ASSETS_BIN:.bin=_code.cpp)
ASSETS_CPP_O := $(ASSETS_CPP:.cpp=.o)

# Build folders
SEG_BUILD_DIR  := $(BUILD_ROOT)/segments
BOOT_BUILD_DIR := $(BUILD_ROOT)/$(PLATFORM_DIR)/boot
BUILD_DIRS     := $(addprefix $(BUILD_ROOT)/,$(SRC_DIRS) $(ASSETS_DIRS)) $(SEG_BUILD_DIR) $(BOOT_BUILD_DIR)

# Build files
C_OBJS   := $(addprefix $(BUILD_ROOT)/,$(C_SRCS:.c=.o))
CXX_OBJS := $(addprefix $(BUILD_ROOT)/,$(CXX_SRCS:.cpp=.o))
ASM_OBJS := $(addprefix $(BUILD_ROOT)/,$(ASM_SRCS:.s=.o))
BIN_OBJS := $(addprefix $(BUILD_ROOT)/,$(BIN_FILES:.bin=.o))
OBJS     := $(C_OBJS) $(CXX_OBJS) $(ASM_OBJS) $(BIN_OBJS) $(ASSETS_CPP_O)
SEG_OBJS := $(addprefix $(BUILD_ROOT)/,$(SEG_C_SRCS:.c=.o)) $(addprefix $(BUILD_ROOT)/,$(SEG_CPP_SRCS:.cpp=.o))
LD_CPP   := $(BUILD_ROOT)/$(LD_SCRIPT)
BOOT_OBJ := $(BUILD_ROOT)/$(BOOT).o
ENTRY_OBJ:= $(BUILD_ROOT)/$(ENTRY_AS:.s=.o)
D_FILES  := $(C_OBJS:.o=.d) $(CXX_OBJS:.o=.d) $(LD_CPP).d $(SEG_OBJS:.o=.d)

CODESEG  := $(BUILD_ROOT)/codesegment.o
Z64      := $(addprefix $(BUILD_ROOT)/,$(TARGET).z64)
V64      := $(addprefix $(BUILD_ROOT)/,$(TARGET).v64)
ELF      := $(Z64:.z64=.elf)

### Flags ###

# Build tool flags

# TODO Add -fdata-sections to CFLAGS and CXXFLAGS
# Doing so will break alignas/alignment attributes, so arrays which require those need to be moved to a separate file
# compilied without those flags

ifeq ($(OS),Windows_NT)
CFLAGS     := -march=vr4300 -mtune=vr4300 -mfix4300 -mabi=32 -mno-shared -G 0 -mhard-float -fno-stack-protector -fno-common -fno-zero-initialized-in-bss \
			  -fno-PIC -mno-abicalls -fno-strict-aliasing -fno-inline-functions -ffreestanding -fwrapv \
			  -mno-check-zero-division -mno-split-addresses -mno-relax-pic-calls -mfp32 -mgp32 -mbranch-cost=1 \
			  -fno-dse -fno-check-pointer-bounds -Wno-chkp -mno-odd-spreg -fno-use-linker-plugin \
        -D_LANGUAGE_C -ffunction-sections
CXXFLAGS     := -march=vr4300 -mtune=vr4300 -mfix4300 -mabi=32 -mno-shared -G 0 -mhard-float -fno-stack-protector -fno-common -fno-zero-initialized-in-bss \
			  -fno-PIC -mno-abicalls -fno-strict-aliasing -fno-inline-functions -ffreestanding -fwrapv \
			  -mno-check-zero-division -mno-split-addresses -mno-relax-pic-calls -mfp32 -mgp32 -mbranch-cost=1 \
			  -fno-dse -fno-check-pointer-bounds -Wno-chkp -mno-odd-spreg -fno-use-linker-plugin \
        -fno-rtti -std=c++20 -D_LANGUAGE_C_PLUS_PLUS -ffunction-sections -fno-exceptions

else
CFLAGS     := -mabi=32 -ffreestanding -G 0 -D_LANGUAGE_C -ffunction-sections -fno-builtin-memset
CXXFLAGS   := -mabi=32 -std=c++20 -fno-rtti -G 0 -D_LANGUAGE_C_PLUS_PLUS -ffunction-sections -fno-exceptions -fno-builtin-memset
endif
CPPFLAGS   := -I include -I $(PLATFORM_DIR)/include -I . -I src/ -Ilib/glm $(SDK_INCLUDE) -D_FINALROM -D_MIPS_SZLONG=32 -D_MIPS_SZINT=32 -D_ULTRA64 -D__EXTENSIONS__ -DF3DEX_GBI_2
WARNFLAGS  := -Wall -Wextra -Wpedantic -Wdouble-promotion -Wfloat-conversion
ASFLAGS    := -mtune=vr4300 -march=vr4300 -mabi=32 -mips3
LDFLAGS    := -T $(LD_CPP) -Wl,--accept-unknown-input-arch -Wl,--no-check-sections -Wl,-Map $(BUILD_ROOT)/$(TARGET).map \
			  $(ULTRA_LINKER) -L lib $(GCC_LINKER) -nostartfiles -Wl,-gc-sections
SEG_LDFLAGS:= $(ULTRA_LINKER) -L lib -lstdc++ -lnustd -l$(LIBULTRA) -e init -Wl,-gc-sections -u numberOfSetBits
LDCPPFLAGS := -P -Wno-trigraphs -DBUILD_ROOT=$(BUILD_ROOT) -Umips
OCOPYFLAGS := --pad-to=0x400000 --gap-fill=0xFF

# TODO test various values of the following:
#  --param max-completely-peeled-insns=X
#  --param max-unrolled-insns=X
#  -finline-limit=X (which controls the following)
#    --param max-inline-insns-single=X/2
#    --param max-inline-insns-auto=X/2
OPT_FLAGS  := -Os -ffast-math # Somehow Os is the fastest option according to testing

ifneq ($(DEBUG),0)
CPPFLAGS     += -DDEBUG_MODE
else
CPPFLAGS     += -DNDEBUG
endif


$(BUILD_ROOT)/platforms/n64/src/usb/usb.o: OPT_FLAGS := -O0
$(BUILD_ROOT)/platforms/n64/src/usb/usb.o: WARNFLAGS += -Wno-unused-variable -Wno-sign-compare -Wno-unused-function
$(BUILD_ROOT)/platforms/n64/src/usb/debug.o: WARNFLAGS += -Wno-unused-parameter -Wno-maybe-uninitialized
$(ASSETS_CPP_O): WARNFLAGS += -Wno-missing-field-initializers
$(SEG_OBJS): WARNFLAGS += -Wno-overflow -Wno-float-conversion -Wno-narrowing -Wno-missing-field-initializers

### Rules ###

# Default target, all
all: $(Z64)

# Make directories
$(BUILD_ROOT) $(BUILD_DIRS) :
	@$(PRINT)$(GREEN)Creating directory: $(ENDGREEN)$(BLUE)$@$(ENDBLUE)$(ENDLINE)
	@$(MKDIR) $@ $(MKDIR_OPTS)

# .cpp -> .o
$(BUILD_ROOT)/%.o : %.cpp | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Compiling C++ source file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(CXX) $< -o $@ -c -MMD -MF $(@:.o=.d) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FLAGS) $(WARNFLAGS)

# .cpp -> .o (build folder)
$(BUILD_ROOT)/%.o : $(BUILD_ROOT)/%.cpp | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Compiling C++ source file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(CXX) $< -o $@ -c -MMD -MF $(@:.o=.d) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FLAGS) $(WARNFLAGS)

# .c -> .o
$(BUILD_ROOT)/%.o : %.c | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Compiling C source file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(CC) $< -o $@ -c -MMD -MF $(@:.o=.d) $(CFLAGS) $(CPPFLAGS) $(OPT_FLAGS) $(WARNFLAGS)

# .bin -> .o
$(BUILD_ROOT)/%.o : %.bin | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Objcopying binary file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(OBJCOPY) -I binary -O elf32-bigmips -B mips:3000 $< $@

# .bin -> .o (build folder)
$(BUILD_ROOT)/%.o : $(BUILD_ROOT)/%.bin | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Objcopying built binary file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(OBJCOPY) -I binary -O elf32-bigmips -B mips:3000 $< $@

# .s -> .o
$(BUILD_ROOT)/%.o : %.s | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Compiling ASM source file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(AS) $< -o $@ $(ASFLAGS)

# boot -> .o
$(BOOT_OBJ) : $(BOOT) | $(BOOT_BUILD_DIR)
	@$(PRINT)$(GREEN)Copying boot to object file$(ENDGREEN)$(ENDLINE)
	@$(OBJCOPY) -I binary -O elf32-big $< $@

# All .o -> codesegment.o
$(CODESEG) : $(OBJS) | $(TOOLS)
	@$(PRINT)$(GREEN)Combining code objects into code segment$(ENDGREEN)$(ENDLINE)
	@$(LD) -o $@ -r $^ $(SEG_LDFLAGS)

# .o -> .elf
$(ELF) : $(CODESEG) $(SEG_OBJS) $(LD_CPP) $(BOOT_OBJ) $(ENTRY_OBJ) $(ASSETS_OBJ)
	@$(PRINT)$(GREEN)Linking elf file:$(ENDGREEN)$(BLUE)$@$(ENDBLUE)$(ENDLINE)
	@$(LD) $(LDFLAGS) -o $@

# .elf -> .z64
$(Z64) : $(ELF)
	@$(PRINT)$(GREEN)Creating z64: $(ENDGREEN)$(BLUE)$@$(ENDBLUE)$(ENDLINE)
	@$(OBJCOPY) $< $@ -O binary $(OCOPYFLAGS)
	@$(PRINT)$(GREEN)Calculating checksums$(ENDGREEN)$(ENDLINE)
	@$(CKSUM) $@
	@$(PRINT)$(WHITE)ROM Built!$(ENDWHITE)$(ENDLINE)

# byteswap z64
$(V64) : $(Z64)
	@$(PRINT)$(GREEN)Byteswapping z64 to v64$(ENDGREEN)$(ENDLINE)
	@$(OBJCOPY) $< $@ -I binary -O binary --reverse-bytes=2

# Preprocess LD script
$(LD_CPP) : $(LD_SCRIPT) | $(BUILD_ROOT)
	@$(PRINT)$(GREEN)Preprocessing linker script$(ENDGREEN)$(ENDLINE)
	@$(CPP) $(LDCPPFLAGS) -MMD -MP -MT $@ -MF $@.d -o $@ $<

# Compile assetpack
$(ASSETPACK) :
	@$(PRINT)$(GREEN)Compiling assetpack$(ENDGREEN)$(ENDLINE)
	@$(MAKE) -C tools/assetpack

# Pack assets
$(ASSETS_BIN) : $(ASSETS_OUT)
	@$(PRINT)$(GREEN)Packing assets$(ENDGREEN)$(ENDLINE)
	@$(ASSETPACK) $(BUILD_ROOT)/$(ASSET_ROOT) $@ $(ASSETS_TXT)

# Create gperf words file
$(ASSETS_GPERF) : $(ASSETS_BIN)
	@$(PRINT)$(GREEN)Creating assets gperf input file$(ENDGREEN)$(ENDLINE)
	@$(CAT) $(PLATFORM_DIR)/asset_gperf_header.txt $(ASSETS_TXT) > $@

# Run gperf
$(ASSETS_CPP) : $(ASSETS_GPERF)
	@$(PRINT)$(GREEN)Running gperf for asset table$(ENDGREEN)$(ENDLINE)
	@$(GPERF) -L C++ -Z FileRecords -N get_offset -C -c -m 1000 -t $< > $@

# Compile gltf64
$(GLTF64) :
	@$(PRINT)$(GREEN)Compiling glTF64$(ENDGREEN)$(ENDLINE)
	@$(MAKE) -C tools/gltf64

# Convert models
$(MODELS_OUT) : $(BUILD_ROOT)/% : %.gltf | $(BUILD_DIRS) $(GLTF64)
	@$(PRINT)$(GREEN)Converting model: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(GLTF64) $< $@ $(BUILD_ROOT)/$(ASSET_ROOT)

# Compile levelconv
$(LEVELCONV) :
	@$(PRINT)$(GREEN)Compiling levelconv$(ENDGREEN)$(ENDLINE)
	@$(MAKE) -C tools/levelconv

# Convert levels
$(LEVELS_OUT) : $(BUILD_ROOT)/% : %.level | $(BUILD_DIRS) $(LEVELCONV)
	@$(PRINT)$(GREEN)Converting level: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(LEVELCONV) $< $@ $(ASSET_ROOT)

clean:
	@$(PRINT)$(YELLOW)Cleaning build$(ENDYELLOW)$(ENDLINE)
	@$(RMDIR) $(BUILD_ROOT) $(RMDIR_OPTS)

load: $(Z64)
	@$(PRINT)$(GREEN)Loading $(Z64) onto flashcart$(ENDGREEN)$(ENDLINE)
	@$(RUN) $(UNFLOADER) $(Z64) -d

.PHONY: all clean load

-include $(D_FILES)

print-% : ; $(info $* is a $(flavor $*) variable set to [$($*)]) @true
