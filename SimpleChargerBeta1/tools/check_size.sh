#!/bin/sh
# Storleksgranser for charger.elf (TunerStudio v2.2, etapp 1).
#   flash = text + data  < 28672 B (28 kB)
#   RAM   = data + bss   < 1229 B  (1,2 kB)
# Motivering [U]: ATmega328P har 32768 B flash varav Optiboot tar 512 B
# (BESLUT nr 2) -> 32256 B for applikationen; 28 kB lamnar ca 3,5 kB marginal.
# RAM ar 2048 B; 1,2 kB statiskt lamnar ca 820 B for stack, vilket ar en
# uppskattad sakerhetsmarginal (stackdjupet mats separat, docs/resurser).
# Anvandning: tools/check_size.sh [charger.elf]
set -eu
ELF="${1:-charger.elf}"
FLASH_MAX=28672
RAM_MAX=1229
set -- $(avr-size -B "$ELF" | awk 'NR==2 {print $1, $2, $3}')
TEXT=$1; DATA=$2; BSS=$3
FLASH=$((TEXT + DATA)); RAM=$((DATA + BSS))
echo "flash $FLASH B (grans < $FLASH_MAX), statiskt RAM $RAM B (grans < $RAM_MAX)"
[ "$FLASH" -lt "$FLASH_MAX" ] || { echo "FEL: flash over gransen"; exit 1; }
[ "$RAM" -lt "$RAM_MAX" ] || { echo "FEL: statiskt RAM over gransen"; exit 1; }
