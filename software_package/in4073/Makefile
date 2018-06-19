PROJECT_NAME := in4073

export OUTPUT_FILENAME
#MAKEFILE_NAME := $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
MAKEFILE_NAME := $(MAKEFILE_LIST)
MAKEFILE_DIR := $(dir $(MAKEFILE_NAME) ) 

TEMPLATE_PATH = ../components/toolchain/gcc
ifeq ($(OS),Windows_NT)
include $(TEMPLATE_PATH)/Makefile.windows
else
include $(TEMPLATE_PATH)/Makefile.posix
endif

MK := mkdir
RM := rm -rf

#echo suspend
ifeq ("$(VERBOSE)","1")
NO_ECHO := 
else
NO_ECHO := @
endif

# Toolchain commands
CC              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-gcc'
AS              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-as'
AR              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ar' -r
LD              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ld'
NM              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-nm'
OBJDUMP         := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objdump'
OBJCOPY         := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objcopy'
SIZE            := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-size'

#function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

#source common to all targets
C_SOURCE_FILES += \
$(abspath ../components/toolchain/system_nrf51.c) \
$(abspath ../components/drivers_nrf/delay/nrf_delay.c) \
$(abspath ./in4073.c) \
$(abspath ./control.c) \
$(abspath ./drivers/gpio.c) \
$(abspath ./freertos/source/rtos_tasks.c) \
$(abspath ./freertos/source/rtos_queue.c) \
$(abspath ./freertos/source/rtos_list.c) \
$(abspath ./freertos/source/rtos_timers.c) \
$(abspath ./freertos/portable/GCC/nrf51/port.c) \
$(abspath ./freertos/source/portable/MemMang/heap_1.c) \
$(abspath ./freertos/portable/CMSIS/nrf51/port_cmsis.c) \
$(abspath ./freertos/portable/CMSIS/nrf51/port_cmsis_systick.c) \
$(abspath ../components/libraries/util/app_error.c) \
$(abspath ../components/libraries/util/nrf_assert.c) \
$(abspath ../components/drivers_nrf/common/nrf_drv_common.c) \
$(abspath ../components/drivers_nrf/clock/nrf_drv_clock.c ) \
$(abspath ../components/drivers_nrf/pstorage/pstorage.c) \
$(abspath ./drivers/uart.c) \
$(abspath ./drivers/queue.c) \
$(abspath ./drivers/adc.c) \
$(abspath ./drivers/twi.c) \
$(abspath ./drivers/timers.c) \
$(abspath ./drivers/baro.c) \
$(abspath ./drivers/spi_flash.c) \
$(abspath ./invensense/inv_mpu.c) \
$(abspath ./invensense/inv_mpu_dmp_motion_driver.c) \
$(abspath ./invensense/ml.c) \
$(abspath ./invensense/mpu_wrapper.c) \
$(abspath ./micro_print.c) \

#$(abspath ../components/libraries/timer/app_timer.c) \

# $(abspath ./drivers/ble.c) \
# $(abspath ../components/softdevice/common/softdevice_handler/softdevice_handler.c) \

# $(abspath ../components/ble/ble_services/ble_nus/ble_nus.c) \
# $(abspath ../components/ble/common/ble_srv_common.c) \
# $(abspath ../components/ble/common/ble_conn_params.c) \
# $(abspath ../components/ble/common/ble_advdata.c) \
# $(abspath ../components/ble/ble_advertising/ble_advertising.c) \


#$(abspath ../components/libraries/timer/app_timer.c) \
# 
# 
# 




#freertos addes stuff





#assembly files common to all targets
ASM_SOURCE_FILES  = $(abspath ../components/toolchain/gcc/gcc_startup_nrf51.s)

#includes common to all targets
INC_PATHS += -I$(abspath ./)
INC_PATHS += -I$(abspath ./invensense)
INC_PATHS += -I$(abspath ./drivers/config)
INC_PATHS += -I$(abspath ./shared)
INC_PATHS += -I$(abspath ../components/device)
INC_PATHS += -I$(abspath ../components/toolchain/gcc)
INC_PATHS += -I$(abspath ../components/toolchain)
INC_PATHS += -I$(abspath ../components/drivers_nrf/hal)
INC_PATHS += -I$(abspath ../components/drivers_nrf/delay)
INC_PATHS += -I$(abspath ../components/softdevice/s110/headers)
INC_PATHS += -I$(abspath ../components/drivers_nrf/config)
INC_PATHS += -I$(abspath ../components/drivers_nrf/clock)
INC_PATHS += -I$(abspath ../components/drivers_nrf/rtc)
INC_PATHS += -I$(abspath ../components/libraries/util)
INC_PATHS += -I$(abspath ../components/ble/common)
INC_PATHS += -I$(abspath ../components/drivers_nrf/pstorage)
INC_PATHS += -I$(abspath ../components/libraries/timer)
INC_PATHS += -I$(abspath ../components/ble/ble_services/ble_nus)
INC_PATHS += -I$(abspath ../components/drivers_nrf/common)
INC_PATHS += -I$(abspath ../components/ble/ble_advertising)
INC_PATHS += -I$(abspath ../components/libraries/trace)
INC_PATHS += -I$(abspath ../components/softdevice/common/softdevice_handler)

#free rtos addes stuff
INC_PATHS += -I$(abspath ./freertos)
INC_PATHS += -I$(abspath ./freertos/source/include)
INC_PATHS += -I$(abspath ./freertos/portable/GCC/nrf51)
INC_PATHS += -I$(abspath ./freertos/portable/CMSIS/nrf51)



OBJECT_DIRECTORY = _build
LISTING_DIRECTORY = $(OBJECT_DIRECTORY)
OUTPUT_BINARY_DIRECTORY = $(OBJECT_DIRECTORY)

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY) $(LISTING_DIRECTORY) )

#flags common to all targets
CFLAGS  = -DNRF51
#CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DS110
CFLAGS += -DBLE_STACK_SUPPORT_REQD
CFLAGS += -mcpu=cortex-m0
CFLAGS += -mthumb -mabi=aapcs --std=gnu11 # abi=Application Binary Interface
CFLAGS += -Wall -Werror -O3
CFLAGS += -mfloat-abi=soft # we don't want floats - no hardware fpu
# keep every function in separate section. This will allow linker to dump unused functions
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin --short-enums
CFLAGS += -DFREERTOS
#CFLAGS += -fstack-usage


# keep every function in separate section. This will allow linker to dump unused functions
LDFLAGS += -Xlinker -Map=$(LISTING_DIRECTORY)/$(OUTPUT_FILENAME).map
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m0
# let linker to dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs -lc -lnosys 
# math library
LDFLAGS += -lm 
LIBS = -lm

# Assembler flags
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -DNRF51
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DS110
ASMFLAGS += -DFREERTOS
ASMFLAGS += -DBLE_STACK_SUPPORT_REQD
#default target - first one defined

default: clean in4073

#building all targets
all: clean
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e cleanobj
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e in4073

#target for printing all targets
help:
	@echo following targets are available:
	@echo 	in4073


C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

ASM_SOURCE_FILE_NAMES = $(notdir $(ASM_SOURCE_FILES))
ASM_PATHS = $(call remduplicates, $(dir $(ASM_SOURCE_FILES) ))
ASM_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(ASM_SOURCE_FILE_NAMES:.s=.o) )

vpath %.c $(C_PATHS)
vpath %.s $(ASM_PATHS)

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

in4073: OUTPUT_FILENAME := in4073
in4073: LINKER_SCRIPT=in4073.ld
in4073: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME).out
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e finalize

## Create build directories
$(BUILD_DIRECTORIES):
#	echo $(MAKEFILE_NAME)
	$(MK) $@

# Create objects from C SRC files
$(OBJECT_DIRECTORY)/%.o: %.c
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC_PATHS) -c -o $@ $<

# Assemble files
$(OBJECT_DIRECTORY)/%.o: %.s
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(ASMFLAGS) $(INC_PATHS) -c -o $@ $<


# Link
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME).out
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out


## Create binary .bin file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	@echo Preparing: $(OUTPUT_FILENAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin

## Create binary .hex file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	@echo Preparing: $(OUTPUT_FILENAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

finalize: genbin genhex echosize

genbin:
	@echo Preparing: $(OUTPUT_FILENAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin

## Create binary .hex file from the .out file
genhex: 
	@echo Preparing: $(OUTPUT_FILENAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

echosize:
	-@echo ''
	$(NO_ECHO)$(SIZE) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	-@echo ''

clean:
	$(RM) $(BUILD_DIRECTORIES)

cleanobj:
	$(RM) $(BUILD_DIRECTORIES)/*.o

upload: default
	dfu_serial/./serial_dfu.py -p $(PORT) _build/in4073.bin

pc: 
	cd ../Ground_Control_Station; python3 GCS.py $(PORT)

pc-run: 
	cd ../Ground_Control_Station; python3 GCS.py $(PORT)
	
upload-run: default pc
	dfu_serial/./serial_dfu.py -p $(PORT) _build/in4073.bin
	cd ../Ground_Control_Station; python3 GCS.py $(PORT)
