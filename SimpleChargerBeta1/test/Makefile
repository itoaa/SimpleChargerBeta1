# Vardtester (PC) och AVR-storleksprov for EEPROM-modulen.
# Paverkar inte firmwarebygget i ../Makefile.
#   make            -> bygg och kor vardtesterna (g++)
#   make avr-probe  -> bygg avr_probe.elf och visa avr-size
#   make golden INI=<sokvag till SmedbyCharger_v2.2.ini> -> generera golden_defaults.inc

CXX      ?= g++
SANITIZE ?= -fsanitize=address,undefined -fno-sanitize-recover=all
CXXFLAGS  = -std=gnu++98 -O1 -g -Wall -Wextra -Werror -I../shared $(SANITIZE)
BUILD     = build

HOST_SRCS = test_eeprom.cpp ../shared/crc16.cpp ../shared/eepromStore.cpp ../shared/eepromDefaults.cpp

AVR_CXX   = avr-g++
AVR_FLAGS = -mmcu=atmega328p -DF_CPU=16000000UL -Os -std=gnu++98 -Wall -Wextra -Werror \
            -ffunction-sections -fdata-sections -I../shared -I../SmeOS/Atmel328p/include
AVR_SRCS  = avr_probe.cpp ../shared/crc16.cpp ../shared/eepromStore.cpp ../shared/eepromDefaults.cpp \
            ../SmeOS/Atmel328p/src/sEeprom.cpp

.PHONY: all test avr-probe golden clean

all: test

test: $(BUILD)/test_eeprom
	./$(BUILD)/test_eeprom

$(BUILD)/test_eeprom: $(HOST_SRCS) golden_defaults.inc $(wildcard ../shared/*.hpp)
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(HOST_SRCS) -o $@

avr-probe: $(BUILD)/avr_probe.elf
	avr-size -B $<

$(BUILD)/avr_probe.elf: $(AVR_SRCS) $(wildcard ../shared/*.hpp)
	mkdir -p $(BUILD)
	$(AVR_CXX) $(AVR_FLAGS) -Wl,--gc-sections $(AVR_SRCS) -o $@

golden:
	python3 gen_defaults_golden.py $(INI) > golden_defaults.inc

clean:
	rm -rf $(BUILD)
