#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

uint8_t cpu_bus_read(uint16_t position);
void cpu_bus_write(uint16_t position, uint8_t data);

uint8_t cpu_bus_read_no_tick(uint16_t position);

#endif
