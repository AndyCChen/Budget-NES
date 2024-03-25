#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// address ranges used by cpu to access cartridge space

#define CPU_CARTRIDGE_START         0x4020
#define CPU_CARTRIDGE_PRG_RAM_START 0x6000
#define CPU_CARTRIDGE_PRG_RAM_END   0x7FFF
#define CPU_CARTRIDGE_PRG_ROM_START 0x8000

// address ranges used by cpu to access ppu registers

#define CPU_PPU_REG_START 0x2000
#define CPU_PPU_REG_END   0x3FFF

// address ranges used by ppu to access cartridge space

#define PPU_CARTRIDGE_PATTERN_TABLE_END 0x1FFF

uint8_t cpu_bus_read(uint16_t position);
void cpu_bus_write(uint16_t position, uint8_t data);

uint16_t cpu_bus_read_u16(uint16_t position);
void cpu_bus_write_u16(uint16_t position, uint16_t data);

#endif