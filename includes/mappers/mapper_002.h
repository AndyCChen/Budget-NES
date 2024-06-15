#ifndef MAPPER_002_H
#define MAPPER_002_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../cartridge.h"

// Mapper 002
// https://www.nesdev.org/wiki/UxROM

typedef struct Registers_002
{
   uint8_t PRG_bank;
} Registers_002;

cartridge_access_mode_t mapper002_cpu_read(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper002_cpu_write(nes_header_t *header, uint16_t position, uint8_t data, size_t *mapped_addr, void* internal_registers);

cartridge_access_mode_t mapper002_ppu_read(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper002_ppu_write(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers);

void mapper002_init(nes_header_t* header, void* internal_registers);

#endif
