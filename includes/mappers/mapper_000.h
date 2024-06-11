#ifndef MAPPER_000_H
#define MAPPER_000_H

#include <stdint.h>
#include <stdbool.h>

#include "../cartridge.h"

cartridge_access_mode_t mapper000_cpu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper000_cpu_write(nes_header_t *header, uint16_t position, uint8_t data, uint16_t *mapped_addr, void* internal_registers);

cartridge_access_mode_t mapper000_ppu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper000_ppu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, void* internal_registers);

void mapper000_init(nes_header_t* header, void* internal_registers);

#endif
