# Vardtester (PC) och AVR-storleksprov for EEPROM-modulen.
# Paverkar inte firmwarebygget i ../Makefile.
#   make            -> bygg och kor alla vardtester (g++, ASan/UBSan)
#                      golden-vektorer for AutoParams genereras fran
#                      ref/auto_params_v2.2.py (kraver python3)
#   make avr-probe  -> bygg avr_probe.elf och visa avr-size
#   make golden INI=<sokvag till SmedbyCharger_v2.2.ini> -> generera golden_defaults.inc

CXX      ?= g++
SANITIZE ?= -fsanitize=address,undefined -fno-sanitize-recover=all
CXXFLAGS  = -std=gnu++98 -O1 -g -Wall -Wextra -Werror -I../shared -I$(BUILD) $(SANITIZE)
BUILD     = build

STORE_SRCS = ../shared/crc16.cpp ../shared/eepromStore.cpp ../shared/eepromDefaults.cpp
HOST_SRCS = test_eeprom.cpp $(STORE_SRCS)
AUTO_SRCS = test_autoparams.cpp ../shared/autoParams.cpp $(STORE_SRCS)
CHG_SRCS  = test_charger.cpp ../shared/charger.cpp ../shared/autoParams.cpp $(STORE_SRCS)
PCMD_SRCS = test_pagecmd.cpp ../shared/pageCommands.cpp ../shared/autoParams.cpp $(STORE_SRCS)

AVR_CXX   = avr-g++
AVR_FLAGS = -mmcu=atmega328p -DF_CPU=16000000UL -Os -std=gnu++98 -Wall -Wextra -Werror \
            -ffunction-sections -fdata-sections -I../shared -I../SmeOS/Atmel328p/include
AVR_SRCS  = avr_probe.cpp ../shared/crc16.cpp ../shared/eepromStore.cpp ../shared/eepromDefaults.cpp \
            ../SmeOS/Atmel328p/src/sEeprom.cpp

.PHONY: all test avr-probe golden clean

all: test

test: $(BUILD)/test_eeprom $(BUILD)/test_autoparams $(BUILD)/test_charger $(BUILD)/test_pagecmd
	./$(BUILD)/test_eeprom
	./$(BUILD)/test_autoparams
	./$(BUILD)/test_charger
	./$(BUILD)/test_pagecmd
	python3 ref/auto_params_v2.2.py > /dev/null

$(BUILD)/test_charger: $(CHG_SRCS) $(wildcard ../shared/*.hpp)
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(CHG_SRCS) -o $@

$(BUILD)/test_pagecmd: $(PCMD_SRCS) $(wildcard ../shared/*.hpp)
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(PCMD_SRCS) -o $@

$(BUILD)/golden_auto.inc: gen_auto_golden.py ref/auto_params_v2.2.py
	mkdir -p $(BUILD)
	python3 gen_auto_golden.py > $@.tmp && mv $@.tmp $@

$(BUILD)/test_autoparams: $(AUTO_SRCS) $(BUILD)/golden_auto.inc $(wildcard ../shared/*.hpp)
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(AUTO_SRCS) -o $@

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
