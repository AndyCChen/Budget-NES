#ifndef MAPPER_001_H
#define MAPPER_001_H

#include <stdint.h>
#include <stdbool.h>

#include "../cartridge.h"

void mapper000_cpu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, cartridge_mode mode);
void mapper000_ppu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, cartridge_mode mode);

void mapper000_cpu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, cartridge_mode mode);
void mapper000_ppu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, cartridge_mode mode);

#endif