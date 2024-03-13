#ifndef MAPPER_001_H
#define MAPPER_001_H

#include <stdint.h>

uint8_t cpu_read(uint16_t position);
uint8_t ppu_read(uint16_t position);

void cpu_write(uint16_t position, uint8_t data);
void ppu_write(uint16_t position, uint8_t data);

#endif