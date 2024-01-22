#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

uint8_t bus_read(uint16_t position);
void bus_write(uint16_t position, uint8_t data);

uint16_t bus_read_u16(uint16_t position);
void bus_write_u16(uint16_t position, uint16_t data);

#endif