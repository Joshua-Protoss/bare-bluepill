
Q := @
###################################################################
# Executables

PREFIX = /c/gcc-arm-none-eabi/bin/arm-none-eabi-

CC := $(PREFIX)gcc
CXX := $(PREFIX)g++
LD := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump

###################################################################
# Source files

BSP_INC_DIR = ../bsp/inc
BSP_SRC_DIR = ../bsp/src
INC_DIR = inc
SRC_DIR = src

BINARY = firmware

OBJS += $(SRC_DIR)/$(BINARY).o
OBJS += $(BSP_SRC_DIR)/startup.o
OBJS += $(BSP_SRC_DIR)/rcc.o
OBJS += $(BSP_SRC_DIR)/gpio.o

###################################################################
# C Flags
DEFS  = -DSTM32F103C8
DEFS  += -I$(INC_DIR)
DEFS  += -I$(BSP_INC_DIR)

CFLAGS = -std=c99 -Os -ggdb3 -Wall
CFLAGS += -mthumb -mcpu=cortex-m3
CFLAGS += $(DEFS)

###################################################################
# Linkerscript

LDSCRIPT = linkerscript.ld
LDFLAGS = -T $(LDSCRIPT) -mcpu=cortex-m3 -mthumb 
LDFLAGS += --static -nostartfiles 
LDFLAGS += -Wl,-Map=firmware.map
###################################################################
###################################################################

all : elf bin

elf : $(BINARY).elf
bin : $(BINARY).bin
images : $(BINARY).images

GENERATED_BINARIES=$(BINARY).elf $(BINARY).bin $(BINARY).map

%.bin : %.elf
	$(Q)$(OBJCOPY) -O binary $< $@

%.elf : $(OBJS)
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o : %.c 
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

clean :
	$(Q)$(RM) -f $(GENERATED_BINARIES) generated.* $(OBJS)

flash : $(BINARY).bin
	openocd -f interface/stlink.cfg \
	        -c "transport select swd" \
	        -c "adapter speed 100" \
	        -f target/stm32f1x.cfg \
	        -c "init" -c "halt" \
	        -c "flash write_image erase firmware.bin 0x08000000" \
	        -c "reset" -c "shutdown"

.PHONY: all clean flash