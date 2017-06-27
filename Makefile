OPENCM3_EXAMPLES_DIR ?= $(HOME)/stm32/libopencm3-examples
OPENCM3_DIR ?= $(OPENCM3_EXAMPLES_DIR)/libopencm3

V = 1

BINARY = main

OBJS = src/usbasp.o src/isp.o

LDSCRIPT = $(OPENCM3_EXAMPLES_DIR)/examples/stm32/f3/stm32f3-discovery/stm32f3-discovery.ld

LIBNAME = opencm3_stm32f3
DEFS += -DSTM32F3

FP_FLAGS ?= -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS = -mthumb -mcpu=cortex-m4 $(FP_FLAGS)

################################################################################
# OpenOCD specific variables

OOCD ?= openocd
OOCD_INTERFACE ?= stlink-v2
OOCD_TARGET ?= stm32f3x

include $(OPENCM3_EXAMPLES_DIR)/examples/rules.mk
