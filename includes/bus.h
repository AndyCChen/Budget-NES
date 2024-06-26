#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

uint8_t cpu_bus_read(uint16_t position);
void cpu_bus_write(uint16_t position, uint8_t data);
void cpu_clear_ram(void);
uint8_t DEBUG_cpu_bus_read(uint16_t position);

#endif
