#==============================================================================
# Makefile for the bare-metal F280049C LED blink example
#
# Requires the TI C2000 code generation tools (cl2000).  Point CG_TOOL_ROOT at
# the compiler installation, e.g.
#
#   make CG_TOOL_ROOT=/opt/ti/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS
#   make CG_TOOL_ROOT=... TARGET=ram
#
# TARGET=flash (default) links for standalone boot from flash.
# TARGET=ram   links for a debugger load into RAM.
#==============================================================================

CG_TOOL_ROOT ?= /opt/ti/ti-cgt-c2000
TARGET       ?= flash
OUT          ?= led_blink_$(TARGET)

CC   := $(CG_TOOL_ROOT)/bin/cl2000
LNK  := $(CG_TOOL_ROOT)/bin/cl2000

CMD  := f280049c_$(TARGET).cmd

CFLAGS := -v28 -ml -mt --float_support=fpu32 --tmu_support=tmu0 \
          --vcu_support=vcrc -O2 --c99 --diag_warning=225 \
          --display_error_number --gen_func_subsections=on

LFLAGS := -z --stack_size=0x200 --entry_point=code_start --rom_model \
          --warn_sections -i"$(CG_TOOL_ROOT)/lib" -i"$(CG_TOOL_ROOT)/include" \
          -m $(OUT).map -o $(OUT).out

SRCS := main.c codestart.asm
OBJS := main.obj codestart.obj

all: $(OUT).out

main.obj: main.c
	$(CC) $(CFLAGS) -c $< --output_file=$@

codestart.obj: codestart.asm
	$(CC) $(CFLAGS) -c $< --output_file=$@

$(OUT).out: $(OBJS) $(CMD)
	$(LNK) $(CFLAGS) $(LFLAGS) $(OBJS) $(CMD) -llibc.a

clean:
	rm -f *.obj *.out *.map

.PHONY: all clean
