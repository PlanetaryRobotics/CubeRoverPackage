include $(BUILD_ROOT)/mk/configs/compiler/include_common.mk
include $(BUILD_ROOT)/mk/configs/compiler/defines_common.mk
include $(BUILD_ROOT)/mk/configs/compiler/defines_freeRTOS.mk
include $(BUILD_ROOT)/mk/configs/compiler/defines_HAL.mk
include $(BUILD_ROOT)/mk/configs/compiler/defines_ccs.mk

# C Compiler
CC :=  $(TI_ARM_COMPILER_BIN_LOC)

CXX := $(CC)

# Coding standard for C
CCS_CFLAGS_COMMON = --c99

# Coding for C++
CCS_CXXFLAGS_COMMON = --c++14

# http://www.ti.com/lit/ug/spnu151r/spnu151r.pdf
# C Compiler flags
CFLAGS := 	-mv7R4 \
			--code_state=32 \
			-me \
			--float_support=VFPv3D16 \
			--diag_warning=225 \
			-g \
			--rtti \
			--cpp_default\
			-O3 \
			--diag_wrap=off \
			--display_error_number \
			--enum_type=packed \
			--abi=eabi \
			--preproc_with_compile \
			--verbose_diagnostics \
			--little_endian \
			$(CCS_CFLAGS_COMMON) \
			$(CCS_CXXFLAGS_COMMON) \
			-I$(BUILD_ROOT) -I$(TI_ARM_COMPILER_INCLUDE_LOC) \
			-I$(FREERTOS_INCLUDES) -I$(FPRIME_FREERTOS_INCLUDES) \
			-I$(FPRIME_FREERTOS_STDTYPES_INCLUDES) \
			-I$(PROJ_ROOT_LOC) \
			-I$(HALCOGEN_INCLUDES)


CXXFLAGS := $(CFLAGS)

DEPEND_FILE := -ppa -ppd=

# Generate object code, don't link (like -c on gcc)
COMPILE_ONLY := -c

# The destination object file (like -o on gcc)
COMPILE_TO := --output_file


# Not the include path itself, but the compiler argument to set it (like -I on gcc)
INCLUDE_PATH = -I

# Linker
AR := $(TI_ARM_ARCHIVER_BIN_LOC)

# Tool to create library
LINK_LIB := $(AR)

##
## To create a library - use flag below
##
LINK_LIB_FLAGS := r

#------------------------------------------------------------------------
# To create a binary - use flags below
#------------------------------------------------------------------------
# argument specifying the file to create the argument
#LIBRARY_TO := -o

# Argument to Library linker to create a binary
#LINK_LIB_FLAGS := 	-mv7R4 \
#					--code_state=32 \
#					--float_support=VFPv3D16 \
#					-g \
#					--diag_warning=225 \
#					--enum_type=packed \
#					--abi=eabi \
#					-z \
#					--reread_libs \
#					--diag_wrap=off \
#					--display_error_number \
#					--heap_size=0x800 \
#					--stack_size=0x800 \
#					--warn_sections \
#					--rom_model \
#					-I$(TI_ARM_LINKER_INCLUDE_LOC) \
#					-I$(TI_ARM_LINKER_LIB_LOC)

# Any post library linking needed (not needed for ar). If not needed, set it to: $(BUILD_ROOT)/mk/bin/empty.py
POST_LINK_LIB :=

POST_LINK_BIN :=

POST_LINK_POST_ARGS = $(subst .out,.bin,$@)

# Link to an executable binary
LINK_BIN := $(CC)

# flag to indicate binary output file
#LINK_BIN_TO :=

# Argument to start/end a list of libraries, if needed
#LIBS_START :=
#LIBS_END :=

# tool that runs on binary, if necessary
#POST_LINK_BIN :=

# Arguments for post linker tool that go before binary file in tool command
#POST_LINK_PRE_ARGS :=

# Arguments for post linker tool that go after binary
#POST_LINK_POST_ARGS :=

# Link to other dependent libraries
#LINK_LIBS :=
