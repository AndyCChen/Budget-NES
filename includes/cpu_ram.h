#ifndef CPU_RAM_H
#define CPU_RAM_H

#include <stdint.h>

#define CPU_RAM_SIZE 0x7ff

uint8_t cpu_ram_read(uint16_t position);
void cpu_ram_write(uint16_t position, uint8_t data);

#endif