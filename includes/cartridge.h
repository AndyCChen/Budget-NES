#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdbool.h>

#define iNES_HEADER_SIZE 16 // iNES headers are all 16 bytes long

bool cartridge_load(const char* const romPath);

uint8_t cartridge_read(uint16_t position);

#endif