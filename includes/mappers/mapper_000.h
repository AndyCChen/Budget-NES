#ifndef MAPPER_001_H
#define MAPPER_001_H

#include <stdint.h>
#include <stdbool.h>

#include "../cartridge.h"

cartridge_access_mode_t mapper000_cpu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr);
cartridge_access_mode_t mapper000_ppu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr);

cartridge_access_mode_t mapper000_cpu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr);
cartridge_access_mode_t mapper000_ppu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr);

#endif
